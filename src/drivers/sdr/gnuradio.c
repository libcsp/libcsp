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

#include <assert.h>
#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>

#include <zmq.h>

#ifdef CSP_POSIX
#include <stdio.h>
#define ex2_log printf
#endif // CSP_POSIX

#define SDR_GNURADIO
#ifdef SDR_GNURADIO

#ifdef CSP_FREERTOS
#define RX_TASK_STACK_SIZE 512
#else
#define RX_TASK_STACK_SIZE 4096
#endif

#define PREAMBLE_B 0xAA
#define SYNCWORD 0x7E
#define PREAMBLE_LEN 16
#define POSTAMBLE_LEN 10
#define PACKET_LEN 128
#define CRC16_LEN 2
#define SYNCWORD_LEN 1
#define LEN_ID_LEN 1

static void *context;
static void *publisher;
static void *subscriber;

uint16_t crc16(char* pData, int length)
{
    uint8_t i;
    uint16_t wCrc = 0xffff;
    while (length--) {
        wCrc ^= *(unsigned char *)pData++ << 8;
        for (i=0; i < 8; i++)
            wCrc = wCrc & 0x8000 ? (wCrc << 1) ^ 0x1021 : wCrc << 1;
    }
    return wCrc & 0xffff;
}

//format and send 128B frame to gnuradio via zmq
static void csp_gnuradio_tx(int fd, const void * data, size_t len){

    if(len != PACKET_LEN){
        while(1); //improve error handling
    }

    //apply framing according to UHF user manual protocol
    uint8_t crc_command[len] = {0};
    crc_command[0] = len;
    for(int i = 0; i < len; i++) {
        crc_command[LEN_ID_LEN+i] = data[i];
    }

    uint16_t crc_res = crc16(crc_command, LEN_ID_LEN + len);

    int radio_len = PREAMBLE_LEN + SYNCWORD_LEN + LEN_ID_LEN + len + CRC16_LEN + POSTAMBLE_LEN;//preamble + sync word + length indicator + data + crc + postamble
    uint8_t radio_command[radio_len] = {0};
    for(int i = 0; i < PREAMBLE_LEN; i++){
      radio_command[i] = PREAMBLE_B;
    }
    for(int i = POSTAMBLE_LEN; i > 0; i--){
        radio_command[radio_len - i] = PREAMBLE_B;
    }

    radio_command[PREAMBLE_LEN] = SYNCWORD;
    radio_command[PREAMBLE_LEN + SYNCWORD_LEN] = len;
    for( int i = 0; i < len; i++) {
        radio_command[PREAMBLE_LEN + SYNCWORD_LEN + CRC16_LEN + i] = data[i];
    }

    radio_command[PREAMBLE_LEN + SYNCWORD_LEN + CRC16_LEN + len] = ((uint16_t)crc_res >> 8) & 0xFF;
    radio_command[PREAMBLE_LEN + SYNCWORD_LEN + CRC16_LEN + len + 1] = ((uint16_t)crc_res >> 0) & 0xFF;

    //send to gnuradio via zmq
    rc = zmq_send(publisher, radio_command, radio_len, 0);
    assert(rc == radio_len);

    return CSP_ERR_NONE;
}

int csp_sdr_tx(const csp_route_t *ifroute, csp_packet_t *packet) {
    csp_iface_t *iface = (csp_iface_t *)ifroute->iface;
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)ifroute->iface->interface_data;
    csp_sdr_conf_t *sdr_conf = (csp_sdr_conf_t *)iface->driver_data;

    if (fec_csp_to_mpdu(ifdata->mac_data, packet, sdr_conf->mtu)) {
        uint8_t *buf;
        int delay_time = sdr_uhf_baud_rate_delay[ifdata->uhf_baudrate];
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
    uint8_t *recv_buf;
    recv_buf = csp_malloc(sdr_conf->mtu);
    csp_packet_t *packet = 0;
    const csp_conf_t *conf = csp_get_conf();

    // Receive loop
    while (1){
        rc = zmq_recv(subscriber, recv_buf, sdr_conf->mtu, 0);//Need to test how/if this is blocking
        //^^^TODO: verify that gnuradio passes frame correctly and sync word and length bytes ignored

        assert(rc != -1);

        bool state = fec_mpdu_to_csp(ifdata->mac_data, recv_buf, &packet, sdr_conf->mtu);
        if (state) {
            ex2_log("%s Rx: received a packet, csp length %d", iface->name, csp_ntoh16(packet->length));
            csp_qfifo_write(packet, iface, NULL);
        }
    }
    return NULL;
}

//Inits zmq tx(pub) and rx(sub)
//starts rx thread
int csp_gnuradio_open(void) {

    context = zmq_ctx_new();

    //Init zmq tx(pub)
    publisher = zmq_socket(context, ZMQ_PUB);
    int rc = zmq_bind(publisher, "tcp://127.0.0.1:1234");
    assert(rc == 0);

    // and rx(sub)
    subscriber = zmq_socket(context, ZMQ_SUB);
    int rc = zmq_connect(subscriber, "tcp://127.0.0.1:4321");
    assert(rc == 0);
    rc = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);
    assert(rc == 0);

    return CSP_ERR_NONE;

}

//Inits zmq tx(pub) and rx(sub)
//starts rx thread
//int csp_sdr_driver_init(const csp_usart_conf_t *conf, csp_usart_callback_t rx_callback, void * user_data,  csp_usart_fd_t* return_fd) {
int csp_sdr_driver_init(csp_iface_t * iface){

    if ((iface == NULL) || (iface->name == NULL) || (iface->interface_data == NULL)) {
        return CSP_ERR_INVAL;
    }
    csp_sdr_interface_data_t *ifdata = (csp_sdr_interface_data_t *)iface->interface_data;
    ifdata->tx_func = (csp_sdr_driver_tx_t) csp_gnuradio_tx;
    csp_sdr_conf_t *sdr_conf = (csp_sdr_conf_t *)iface->driver_data;

    ifdata->rx_queue = csp_queue_create(((csp_sdr_conf_t *)iface->driver_data)->mtu, 1);
    csp_thread_create(csp_gnuradio_rx_task, "gnuradio_rx", RX_TASK_STACK_SIZE, (void *)iface, 0, NULL);

    int res = csp_gnuradio_open();
    if (res != CSP_ERR_NONE) {
        csp_free(conf);
        return res;
    }

    return 0;
}
#endif /* SDR_GNURADIO */
