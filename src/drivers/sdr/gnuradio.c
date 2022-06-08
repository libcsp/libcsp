/*
 * Copyright (C) 2021  University of Alberta
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/**
 * @file gnuradio.c
 * @author Josh Lazaruk
 * @date 2022-05-12
 */
#include <csp/csp.h>

#ifdef CSP_POSIX
#include <stdio.h>
#define ex2_log printf
#endif // CSP_POSIX

#ifdef SDR_GNURADIO
#include <assert.h>
#include <csp/csp_interface.h>
#include <csp/interfaces/csp_if_sdr.h>
#include <csp/csp_endian.h>
#include <csp/drivers/usart.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>
#include <csp/arch/csp_queue.h>
#include <csp/arch/csp_semaphore.h>
#include <csp/drivers/sdr.h>
#include "csp/drivers/fec.h"
#include <netdb.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SA struct sockaddr

#include <zmq.h>

#define RX_TASK_STACK_SIZE 4096

#define PREAMBLE_B 0xAA
#define SYNCWORD 0x7E
#define PREAMBLE_LEN 16
#define POSTAMBLE_LEN 10
#define PACKET_LEN 128
#define CRC16_LEN 2
#define SYNCWORD_LEN 1
#define LEN_ID_LEN 1
#define RADIO_LEN PREAMBLE_LEN + SYNCWORD_LEN + LEN_ID_LEN + PACKET_LEN + CRC16_LEN + POSTAMBLE_LEN
//^^radio_len = preamble + sync word + length indicator + data + crc + postamble

// static void *context;
// static void *publisher;
// static void *subscriber;
// static int rc;
static int sockfd;

uint16_t crc16(uint8_t * pData, int length)
{
    uint8_t i;
    uint16_t wCrc = 0xffff;
    while (length--) {
        wCrc ^= *(unsigned char *)pData++ << 8;
        for (i=0; i < 8; i++)
            wCrc = wCrc & 0x8000 ? (wCrc << 1) ^ 0x1021 : wCrc << 1;
    }
    printf("done crc16\n");
    return wCrc & 0xffff;
}

//format and send 128B frame to gnuradio via zmq
int csp_gnuradio_tx(int fd, const void * data, size_t len){
    
    if((int)len != PACKET_LEN){
        while(1); //improve error handling
    }
    //apply framing according to UHF user manual protocol
    uint8_t crc_command[LEN_ID_LEN + PACKET_LEN] = {0};
    crc_command[0] = PACKET_LEN;
    
    printf("129B packet: ");

    for(int i = 0; i < PACKET_LEN; i++) {
        crc_command[LEN_ID_LEN+i] = ((uint8_t *)data)[i];
        printf("%02X ", crc_command[i]);
    }
    printf("%02X ", crc_command[PACKET_LEN]);
        
    printf("\n");

    uint16_t crc_res = crc16(crc_command, LEN_ID_LEN + PACKET_LEN);

    uint8_t radio_command[RADIO_LEN] = {0};
    for(int i = 0; i < PREAMBLE_LEN; i++){
      radio_command[i] = PREAMBLE_B;
      
    }

    for(int i = POSTAMBLE_LEN; i > 0; i--){
        radio_command[RADIO_LEN - i] = PREAMBLE_B;
    }

    radio_command[PREAMBLE_LEN] = SYNCWORD;
    radio_command[PREAMBLE_LEN + SYNCWORD_LEN] = PACKET_LEN;
    for( int i = 0; i < PACKET_LEN; i++) {
        radio_command[PREAMBLE_LEN + SYNCWORD_LEN + LEN_ID_LEN + i] = ((uint8_t *)data)[i];
    }

    radio_command[PREAMBLE_LEN + SYNCWORD_LEN + LEN_ID_LEN + PACKET_LEN] = ((uint16_t)crc_res >> 8) & 0xFF;
    radio_command[PREAMBLE_LEN + SYNCWORD_LEN + LEN_ID_LEN + PACKET_LEN + 1] = ((uint16_t)crc_res >> 0) & 0xFF;
    
    //send to radio via tcp
    printf("sending csp packet to gnuradio\n");
    FILE *fptr = fopen("output2.bin","w");
    fwrite(radio_command, sizeof(uint8_t), RADIO_LEN, fptr);
    fclose(fptr);
    int status = system("cat output2.bin | nc -w 1 127.0.0.1 1235");
    if(status == -1){
        printf("System call failed\n");
        while(1);
    }

    return CSP_ERR_NONE;
}

int csp_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet) {
    csp_iface_t *iface = (csp_iface_t *)ifroute->iface;
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)ifroute->iface->interface_data;
    csp_sdr_conf_t *sdr_conf = (csp_sdr_conf_t *)iface->driver_data;

    if (fec_csp_to_mpdu(ifdata->mac_data, packet, sdr_conf->mtu)) {
        uint8_t *buf;
        int delay_time = 0;
        size_t mtu = (size_t)fec_get_next_mpdu(ifdata->mac_data, (void **)&buf);
        while (mtu != 0) {
            (ifdata->tx_func)(ifdata->fd, buf, mtu);
            mtu = fec_get_next_mpdu(ifdata->mac_data, (void **)&buf);
            csp_sleep_ms(delay_time);
        }
    }
    return CSP_ERR_NONE;
}

CSP_DEFINE_TASK(csp_gnuradio_rx_task) {
    csp_iface_t *iface = (csp_iface_t *)param;
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)iface->interface_data;
    csp_sdr_conf_t *sdr_conf = (csp_sdr_conf_t *)iface->driver_data;
    uint8_t buflen = sdr_conf->mtu + 1;//+1 weirdness to discard "length" field
    uint8_t * recv_buf = csp_malloc(buflen);
    csp_packet_t *packet = 0;

    // Receive loop
    while (1){
        bzero(recv_buf, buflen);
        int status = read(sockfd, recv_buf, buflen);
        if(status == -1){
            printf("tcp read failed\n");
            while(1);
        }

        if(recv_buf[0] == 128){//length indicator byte signifying full-length packet
            bool state = fec_mpdu_to_csp(ifdata->mac_data, recv_buf + 1, &packet, sdr_conf->mtu);
            if (state) {
                ex2_log("%s Rx: received a packet, csp length %d", iface->name, csp_ntoh16(packet->length));
                csp_qfifo_write(packet, iface, NULL);
            }
        }


    }
    return NULL;
}

//Inits tcp tx and rx
//starts rx thread
int csp_gnuradio_open(void) {

    struct sockaddr_in servaddr;
   
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("TCP socket w/ gnuradio creation failed...\n");
        exit(0);
    }
    else
        printf("TCP socket w/ gnuradio successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(4321);
   
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the gnuradio TCP server failed...\n");
        while(1);
    }
    else
        printf("Connected to the gnuradio TCP server..\n");

    return CSP_ERR_NONE;

}

int csp_sdr_driver_init(csp_iface_t * iface){

    if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
        return CSP_ERR_INVAL;
    }
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)iface->interface_data;
    ifdata->tx_func = (csp_sdr_driver_tx_t) csp_gnuradio_tx;

    ifdata->rx_queue = csp_queue_create(((csp_sdr_conf_t *)iface->driver_data)->mtu, 1);
    int res = csp_gnuradio_open();
    if (res != CSP_ERR_NONE) {
        return res;
    }
    csp_thread_create(csp_gnuradio_rx_task, "gnuradio_rx", RX_TASK_STACK_SIZE, (void *)iface, 0, NULL);

    return 0;
}
#endif /* SDR_GNURADIO */
