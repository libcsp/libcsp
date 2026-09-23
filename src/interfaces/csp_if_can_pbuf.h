#pragma once

#include <csp/csp_types.h>
#include <csp/interfaces/csp_if_can.h>

void csp_can_pbuf_free(csp_can_interface_data_t * ifdata, csp_packet_t * buffer, int buf_free, int * task_woken);
csp_packet_t * csp_can_pbuf_new(csp_can_interface_data_t * ifdata, uint32_t id, csp_id_t csp_id, int * task_woken);
csp_packet_t * csp_can_pbuf_find(csp_can_interface_data_t * ifdata, uint32_t id, uint32_t mask, int * task_woken);
void csp_can_pbuf_cleanup(csp_can_interface_data_t * ifdata, int * task_woken);
