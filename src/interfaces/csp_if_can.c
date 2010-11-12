/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2010 GomSpace ApS (gomspace.com)
Copyright (C) 2010 AAUSAT3 Project (aausat3.space.aau.dk) 

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/* CSP CAN Interface for SocketCAN hosts
 * 2010 Jeppe Ledet-Pedersen/AAUSAT3 Project
 */
 
/* CAN Fragmentation Protocol Header:
 * src:             5 bits
 * dst:             5 bits
 * type:            1 bit
 * remain:          8 bits
 * identifier:     10 bits
 *
 * The header matches the 29 bit extended CAN identifier.
 * source and destination addresses are the same as in CSP
 * type is BEGIN (0) for the first fragment and MORE (1) for remaining
 * remain indicates number of remaining fragments
 * identifier is an auto incrementing integer to uniquely separate sessions
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/socket.h>
#include <bits/socket.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>
#include <csp/csp_endian.h>

/** CAN header macros */
#define CFP_HOST_SIZE 4
#define CFP_TYPE_SIZE 1
#define CFP_REMAIN_SIZE 13
#define CFP_ID_SIZE 7

#define CFP_SRC(id) ((uint8_t)(((id) >> (CFP_HOST_SIZE + CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE)) & ((1 << CFP_HOST_SIZE) - 1)))
#define CFP_DST(id) ((uint8_t)(((id) >> (CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE)) & ((1 << CFP_HOST_SIZE) - 1)))
#define CFP_TYPE(id) ((uint8_t)(((id) >> (CFP_REMAIN_SIZE + CFP_ID_SIZE)) & ((1 << CFP_TYPE_SIZE) - 1)))
#define CFP_REMAIN(id) ((uint8_t)(((id) >> (CFP_ID_SIZE)) & ((1 << CFP_REMAIN_SIZE) - 1)))
#define CFP_ID(id) ((uint8_t)(((id) >> (0)) & ((1 << CFP_ID_SIZE) - 1)))

#define CFP_MAKE_SRC(id) (((id) & ((1 << CFP_HOST_SIZE) - 1)) << (CFP_HOST_SIZE + CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE))
#define CFP_MAKE_DST(id) (((id) & ((1 << CFP_HOST_SIZE) - 1)) << (CFP_TYPE_SIZE + CFP_REMAIN_SIZE + CFP_ID_SIZE))
#define CFP_MAKE_TYPE(id) (((id) & ((1 << CFP_TYPE_SIZE) - 1)) << (CFP_REMAIN_SIZE + CFP_ID_SIZE))
#define CFP_MAKE_REMAIN(id) (((id) & ((1 << CFP_REMAIN_SIZE) - 1)) << (CFP_ID_SIZE))
#define CFP_MAKE_ID(id) (((id) & ((1 << CFP_ID_SIZE) - 1)) << 0)

#define CFP_ID_CONN_MASK (CFP_MAKE_SRC((1 << CFP_HOST_SIZE) - 1) | CFP_MAKE_DST((1 << CFP_HOST_SIZE) - 1) | CFP_MAKE_ID((1 << CFP_ID_SIZE) - 1))

#define CSP_CAN_MTU 256
#define THREAD_POOL_SIZE 5

/* These constants are not defined for Blackfin */
#if !defined(PF_CAN) && !defined(AF_CAN)
    #define PF_CAN 29
    #define AF_CAN PF_CAN
#endif

/** CFP Frame Types */
enum cfp_frame_t {
    CFP_BEGIN = 0,
    CFP_MORE = 1
};

int can_socket; /** SocketCAN socket handle  */
int cfp_id;     /** CFP identifier           */
sem_t tp_sem;   /** Thread pool semaphore    */
sem_t pbuf_sem; /** Packet buffer semaphore  */
sem_t id_sem;   /** CFP identifier semaphore */

/* tx thread prototype */
static void * can_tx_thread(void * parameters);

/* Identification number */
void id_init(void) {
    /* Init ID field to random number */
    srand(time(0));
    cfp_id = rand() & ((1 << CFP_ID_SIZE) - 1);
    
    if (sem_init(&id_sem, 0, 1) != 0)
        perror("sem_init");
}

int id_get(void) {
    int id;
    sem_wait(&id_sem);
    id = cfp_id++;
    cfp_id = cfp_id & ((1 << CFP_ID_SIZE) - 1);
    sem_post(&id_sem);
    return id;
}

/* Thread pool */
typedef enum {
    THREAD_FREE = 0,
    THREAD_USED = 1,
} tp_state_t;

typedef struct {
    pthread_t tp_thread;    /** Thread handle */
    tp_state_t state;       /** Thread state */
    uint32_t cfpid;         /** CFP identifier */
    csp_packet_t * packet;  /** CSP packet */
    sem_t * tx_sem;         /** TX sempaphore used for blocking calls */
    sem_t signal_sem;       /** Signalling semaphore */
} tp_element_t;

static tp_element_t tp[THREAD_POOL_SIZE];

static int tp_init(void) {

    uint8_t i = 0;
    for (i = 0; i < THREAD_POOL_SIZE; i++) {
        tp[i].state = THREAD_FREE;
        tp[i].cfpid = 0;
        tp[i].packet = NULL;
        tp[i].tx_sem = NULL;

        /* Init signal semaphore */
        if (sem_init(&(tp[i].signal_sem), 0, 1) != 0) {
            perror("sem_init");
            return -1;
        } else {
            sem_wait(&(tp[i].signal_sem));
        }

        /* Create thread */
        if (pthread_create(&tp[i].tp_thread, NULL, can_tx_thread, (void *)&tp[i]) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    /* Init thread pool semaphore */
    sem_init(&tp_sem, 0, 1);

    return 0;
    
}

static tp_element_t * tp_get_thread(void) {
    
    uint8_t i;
    tp_element_t * t;
   
    sem_wait(&tp_sem);
    for (i = 0; i < THREAD_POOL_SIZE; i++) {
        t = &tp[i];

        if(t->state == THREAD_FREE) {
            t->state = THREAD_USED;
            sem_post(&tp_sem);
            return t;
        }
    }
    sem_post(&tp_sem);

    /* No free thread */
    return NULL;
}

static void tp_free_thread(tp_element_t * t) {
    t->cfpid = 0;
    t->packet = NULL;
    t->tx_sem = NULL;
    sem_wait(&tp_sem);
    t->state = THREAD_FREE;
    sem_post(&tp_sem);
}

/* Packet buffers */
typedef enum {
    BUF_FREE = 0,
    BUF_USED = 1,
} pbuf_state_t;

typedef struct {
    uint16_t counter;
    uint32_t cfpid;
    csp_packet_t * packet;
    pbuf_state_t state;
} pbuf_element_t;

static pbuf_element_t pbuf[CSP_CONN_MAX];

static void pbuf_init(void) {

    /* Initialize packet buffers */
    int i;
    pbuf_element_t * buf;

    for (i = 0; i < CSP_CONN_MAX; i++) {
        buf = &pbuf[i];
        buf->counter = 0;
        buf->state = BUF_FREE;
        buf->packet = NULL;
        buf->cfpid = 0;
    }

    /* Init packet buffer semaphore */
    sem_init(&pbuf_sem, 0, 1);

    return;
    
}

static pbuf_element_t * pbuf_new(uint32_t id) {

    /* Search for free buffer */
    int i;
    pbuf_element_t * buf;

    sem_wait(&pbuf_sem);
    for (i = 0; i < CSP_CONN_MAX; i++) {
        buf = &pbuf[i];

        if(buf->state == BUF_FREE) {
            buf->state = BUF_USED;
            buf->cfpid = id;
            sem_post(&pbuf_sem);
            return buf;
        }
    }
    sem_post(&pbuf_sem);

    /* No free buffer was found */
    return NULL;
  
}

/* Find matching packet buffer or create a new one */
static pbuf_element_t * pbuf_find(uint32_t id, uint32_t mask) {
    
    /* Search for matching buffer */
    int i;
    pbuf_element_t * buf;

    sem_wait(&pbuf_sem);
    for (i = 0; i < CSP_CONN_MAX; i++) {
        buf = &pbuf[i];

        if((buf->state == BUF_USED) && ((buf->cfpid & mask) == (id & mask))) {
            sem_post(&pbuf_sem);
            return buf;
        }
    }
    sem_post(&pbuf_sem);

    /* If no mathing buffer was found, try to create a new one */
    return pbuf_new(id);

}

static void pbuf_free(pbuf_element_t * buf) {

    if (buf->packet != NULL) {
        csp_buffer_free(buf->packet);
        buf->packet = NULL;
    }
    sem_wait(&pbuf_sem);
    buf->state = BUF_FREE;
    sem_post(&pbuf_sem);
    
    return;

}

/* TX thread. This works as a pseudo-mob for csp_can_tx */
static void * can_tx_thread(void * parameters) {

    struct can_frame frame;
    uint8_t cpt, bytes, length, id, overhead;

    /* Set thread parameters */
    tp_element_t * param = (tp_element_t *)parameters;
    
    /* Calculate overhead */
    overhead = sizeof(csp_id_t) + sizeof(uint16_t);
    
    while (1) {

        /* Wait for a new packet to process */
        sem_wait(&(param->signal_sem));

        length = param->packet->length;
        cpt = 0;
        
        /* Get valid identifier */
        id = id_get();

        do {
            /* Prepare identifier */
            frame.can_id  = 0;
            frame.can_id |= CFP_MAKE_SRC(param->packet->id.src);
            frame.can_id |= CFP_MAKE_DST(param->packet->id.dst);
            frame.can_id |= CFP_MAKE_ID(id);
            
            if (cpt == 0) {
                /* First frame */
                frame.can_id |= CFP_MAKE_TYPE(CFP_BEGIN);
                frame.can_id |= CFP_MAKE_REMAIN((length + 7) / 8);

                /* Calculate frame data bytes */                
                if (length <= 8 - overhead)
                    bytes = length;
                else
                    bytes = 8 - overhead;
                
                /* Copy CSP headers and data */
                uint32_t csp_id_be = htonl(param->packet->id.ext);
                uint16_t csp_length_be = htons(param->packet->length);
                
                memcpy(frame.data, &csp_id_be, sizeof(csp_id_be));
                memcpy(frame.data + sizeof(csp_id_be), &csp_length_be, sizeof(csp_length_be)); 
                memcpy(frame.data + overhead, param->packet->data, bytes);   
                frame.can_dlc = bytes + overhead;
            } else {
                /* Following frames */
                frame.can_id |= CFP_MAKE_TYPE(CFP_MORE);
                frame.can_id |= CFP_MAKE_REMAIN((length - cpt) / 8);
                
                /* Calculate frame data bytes */
                if (length - cpt >= 8)
                    bytes = 8;
                else
                    bytes = length - cpt;
                
                /* Copy data */
                memcpy(frame.data, param->packet->data + cpt, bytes);
                frame.can_dlc = bytes;
            }
            
            /* Add number of bytes sent to counter */
            cpt += bytes; 

            /* Set extended frame format */
            frame.can_id |= CAN_EFF_FLAG;

            /* Send frame */
            uint8_t tries = 0;
            uint8_t error = 0;
            while (write(can_socket, &frame, sizeof(frame)) != sizeof(frame)) {
                if (++tries < 10 && errno == ENOBUFS) {
                    /* Wait 1 ms and try again */
                    usleep(1000);
                } else {
                    perror("write");
                    error = 1;
                    break;
                }
            }
            
            /* Abort transmission if an error occured */
            if (error)
                break;
                
        } while (length > cpt);

        /* Free packet */
        if (param->packet != NULL)
            csp_buffer_free(param->packet);
        
        /* Post semaphore if blocking mode is enabled */
        if (param->tx_sem != NULL)
            sem_post(param->tx_sem);

        /* Ready for a new packet */
        tp_free_thread(param);

    }
    
    pthread_exit(NULL);

}

/** TX interface */
int csp_can_tx(csp_id_t cspid, csp_packet_t * packet, unsigned int timeout) {

    sem_t * tx_sem;
    
    /* Create parameter struct */
    tp_element_t * tp_thread = tp_get_thread();
    if (tp_thread == NULL) {
        printf("TX overflow: No available outgoing thread\n");
        return 0;
    }

    /* Create tx semaphore if blocking mode is enabled */
    if (timeout != 0) {
        tx_sem = malloc(sizeof(sem_t));
        if (tx_sem == NULL) {
            tp_free_thread(tp_thread);
            printf("Failed to allocate tx semaphore\n");
            return 0;
        } else {
            sem_init(tx_sem, 0, 1);
            /* This will always succeed */
            sem_trywait(tx_sem);
            tp_thread->tx_sem = tx_sem;
        }
    } else {
        tx_sem = NULL;
        tp_thread->tx_sem = NULL;
    }

    /* Copy identifier */
    tp_thread->packet = packet;
    tp_thread->packet->id = cspid;

    /* Signal thread to start */
    sem_post(&(tp_thread->signal_sem));

    /* Non blocking mode */
    if (timeout == 0)
        return 1;

    /* Blocking mode */
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts))
        return 0;

    uint32_t sec = timeout / 1000;
    uint32_t nsec = (timeout - 1000 * sec) * 1000000;
    
    ts.tv_sec += sec;
    
    if (ts.tv_nsec + nsec > 1000000000)
        ts.tv_sec++;
    
    ts.tv_nsec = (ts.tv_nsec + nsec) % 1000000000;

    /* Wait for post from tx thread */
    if (sem_timedwait(tx_sem, &ts) != 0) {
        sem_post(tx_sem);
        return 0;
    } else {
        sem_post(tx_sem);
        return 1;
    }
    
}

/** csp_rx_handler
 * This is the RX handler that is run when CAN frames are received 
 */
static void rx_handler(struct can_frame * frame) {

    static pbuf_element_t * buf;
    uint8_t offset;
    
    uint32_t id = frame->can_id & CAN_EFF_MASK;

    /* A little debugging information please */
    csp_debug(CSP_DEBUG, "CAN Frame src=%#02x dst=%#02x type=%#02x remain=%#02x id=%#02x dlc=%#02x\n",
        CFP_SRC(id),
        CFP_DST(id),
        CFP_TYPE(id),
        CFP_REMAIN(id),
        CFP_ID(id),
        frame->can_dlc
    );

    /* Bind incoming frame to a packet buffer */
    buf = pbuf_find(id, CFP_ID_CONN_MASK);

    /* Check returned buffer */
    if (buf == NULL) {
        csp_debug(CSP_INFO, "No available packet buffer for CAN\n");
        return;
    }
    
    /* Reset frame data offset */
    offset = 0;

    switch (CFP_TYPE(id)) {

        case CFP_BEGIN:
        
            /* Discard packet if DLC is less than CSP id + CSP length fields */
            if (frame->can_dlc < sizeof(csp_id_t) + sizeof(uint16_t)) {
                printf("Warning: Short BEGIN frame received\r\n");
                pbuf_free(buf);
                break;
            }
                        
            /* Check for incomplete frame */
            if (buf->packet != NULL) {
                printf("Warning: Incomplete frame\r\n");
            } else {
                /* Allocate memory for frame */
                buf->packet = csp_buffer_get(CSP_CAN_MTU);
                if (buf->packet == NULL) {
                    printf("Failed to get buffer for CSP_BEGIN packet\n");
                    break;
                }
            }
            
            /* Copy CSP identifier and length*/
            memcpy(&(buf->packet->id), frame->data, sizeof(csp_id_t));
            buf->packet->id.ext = ntohl(buf->packet->id.ext);
            memcpy(&(buf->packet->length), frame->data + sizeof(csp_id_t), sizeof(uint16_t));
            buf->packet->length = ntohs(buf->packet->length);
            
            buf->counter = 0;
            
            /* Set offset to prevent CSP header from being copied to CSP data */
            offset = sizeof(csp_id_t) + sizeof(uint16_t);

            /* Note fall through! */
            
        case CFP_MORE:

            /* Check for overflow */
            if ((buf->counter + frame->can_dlc - offset) > buf->packet->length) {
                printf("RX buffer overflow!\r\n");
                pbuf_free(buf);
                break;
            }

            /* Copy dlc bytes into buffer */
            memcpy(&buf->packet->data[buf->counter], frame->data + offset, frame->can_dlc - offset);
            buf->counter += frame->can_dlc - offset;

            /* Check if more data is expected */
            if (buf->counter != buf->packet->length)
                break;

            /* Data is available */
            csp_debug(CSP_DEBUG, "Full packet received\n");
            csp_new_packet(buf->packet, csp_can_tx, NULL);
            buf->packet = NULL;

            /* Free packet buffer */
            pbuf_free(buf);

            break;

        default:
            printf("Unknown CFP message type\r\n");
            pbuf_free(buf);
            break;
    }

    return;

}

static void * can_rx_thread(void * parameters) {

    struct can_frame frame;
    int nbytes;

    while (1) {
        /* Read CAN frame */
        nbytes = read(can_socket, &frame, sizeof(frame));

        if (nbytes != sizeof(frame)) {
            printf("Read incomplete CAN frame\n");
            continue;
        }
        
        rx_handler(&frame);
    }
    
    printf("Exiting receive thread\n");
    
    pthread_exit(NULL);

}

int csp_can_init(char * ifc, uint8_t myaddr, uint8_t promisc) {

    struct ifreq ifr;
    struct sockaddr_can addr;
    pthread_t rx_thread;

    /* Init packet buffer */
    pbuf_init();
    
    /* Initialze CFP identifier */
    id_init();

    /* Create socket */
    if ((can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("socket");
        return 0;
    }

    /* Locate interface */
    strncpy(ifr.ifr_name, ifc, IFNAMSIZ - 1);
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        return 0;
    }

    /* Bind the socket to CAN interface */
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 0;
    }
    
    /* Set promiscuous mode */
    if (!promisc) {
        struct can_filter filter;
        filter.can_id   = CFP_MAKE_DST(myaddr);
        filter.can_mask = CFP_MAKE_DST((1 << CFP_HOST_SIZE) - 1);
        if (setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
            //perror("setsockopt");
            return 0;
        }
    }

    /* Create receive thread */
    if (pthread_create(&rx_thread, NULL, can_rx_thread, NULL) != 0) {
        perror("pthread_create");
        return 0;
    }
    
    /* Create TX thread pool */
    if (tp_init() != 0) {
        printf("Failed to create tx thread pool\n");
        return 0;
    }
    
    return 1;

}

