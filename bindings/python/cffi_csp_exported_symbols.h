enum csp_reserved_ports_e {
    CSP_CMP,
    CSP_PING,
    CSP_PS,
    CSP_MEMFREE,
    CSP_REBOOT,
    CSP_BUF_FREE,
    CSP_UPTIME,
    CSP_ANY,
    CSP_PROMISC,
};

typedef enum {
    CSP_PRIO_CRITICAL,
    CSP_PRIO_HIGH,
    CSP_PRIO_NORM,
    CSP_PRIO_LOW,
} csp_prio_t;

enum {
    CSP_SO_NONE,
    CSP_SO_RDPREQ,
    CSP_SO_RDPPROHIB,
    CSP_SO_HMACREQ,
    CSP_SO_HMACPROHIB,
    CSP_SO_XTEAREQ,
    CSP_SO_XTEAPROHIB,
    CSP_SO_CRC32REQ,
    CSP_SO_CRC32PROHIB,
    CSP_SO_CONN_LESS,
    CSP_NODE_MAC,
    CSP_ROUTE_FIFOS,
    CSP_RX_QUEUES,
    CSP_ID_PRIO_SIZE,
    CSP_ID_HOST_SIZE,
    CSP_ID_PORT_SIZE,
    CSP_ID_FLAGS_SIZE,
    CSP_DEFAULT_ROUTE,
};

typedef union {
    uint32_t ext;
    ...;
} csp_id_t;

typedef struct {
    uint16_t length;
    csp_id_t id;
    union {
        uint8_t data[0];
        ...;
    };
    ...;
} csp_packet_t;

typedef struct csp_iface_s {
    const char *name;           /**< Interface name (keep below 10 bytes) */
    ...;
} csp_iface_t;

typedef struct csp_conn_s csp_conn_t;
typedef struct csp_conn_s csp_socket_t;
typedef void * csp_memptr_t;
typedef csp_memptr_t (*csp_memcpy_fnc_t)(csp_memptr_t, const csp_memptr_t, size_t);

/* Functions */
csp_conn_t *csp_accept(csp_socket_t *socket, uint32_t timeout);
extern void  csp_assert_fail_action(char *assertion, const char *file, int line);
uint16_t csp_betoh16(uint16_t be16);
uint32_t csp_betoh32(uint32_t be32);
uint64_t csp_betoh64(uint64_t be64);
int csp_bind(csp_socket_t *socket, uint8_t port);
int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority, csp_iface_t * _if_a, csp_iface_t * _if_b);
void csp_buf_free(uint8_t node, uint32_t timeout);
void * csp_buffer_clone(void *buffer);
void csp_buffer_free(void *packet);
void csp_buffer_free_isr(void *packet);
void * csp_buffer_get(size_t size);
void * csp_buffer_get_isr(size_t buf_size);
int csp_buffer_init(int count, int size);
int csp_buffer_remaining(void);
int csp_buffer_size(void);
int csp_can_init(uint8_t mode, struct csp_can_config *conf);
int csp_close(csp_conn_t *conn);
void csp_cmp_set_memcpy(csp_memcpy_fnc_t fnc);
int csp_conn_dport(csp_conn_t *conn);
int csp_conn_dst(csp_conn_t *conn);
int csp_conn_flags(csp_conn_t *conn);
void csp_conn_print_table(void);
int csp_conn_sport(csp_conn_t *conn);
int csp_conn_src(csp_conn_t *conn);
csp_conn_t *csp_connect(uint8_t prio, uint8_t dest, uint8_t dport, uint32_t timeout, uint32_t opts);
char *csp_get_hostname(void);
char *csp_get_model(void);
uint16_t csp_htobe16(uint16_t h16);
uint32_t csp_htobe32(uint32_t h32);
uint64_t csp_htobe64(uint64_t h64);
uint16_t csp_htole16(uint16_t h16);
uint32_t csp_htole32(uint32_t h32);
uint64_t csp_htole64(uint64_t h64);
uint16_t csp_hton16(uint16_t h16);
uint32_t csp_hton32(uint32_t h32);
uint64_t csp_hton64(uint64_t h64);
double csp_htondbl(double d);
float csp_htonflt(float f);
void csp_iflist_add(csp_iface_t *ifc);
csp_iface_t * csp_iflist_get_by_name(char *name);
void csp_iflist_print(void);
int csp_init(uint8_t my_node_address);
uint16_t csp_letoh16(uint16_t le16);
uint32_t csp_letoh32(uint32_t le32);
uint64_t csp_letoh64(uint64_t le64);
int csp_listen(csp_socket_t *socket, size_t conn_queue_length);
void csp_memfree(uint8_t node, uint32_t timeout);
uint16_t csp_ntoh16(uint16_t n16);
uint32_t csp_ntoh32(uint32_t n32);
uint64_t csp_ntoh64(uint64_t n64);
double csp_ntohdbl(double d);
float csp_ntohflt(float f);
int csp_ping(uint8_t node, uint32_t timeout, unsigned int size, uint8_t conn_options);
void csp_ping_noreply(uint8_t node);
void csp_ps(uint8_t node, uint32_t timeout);
csp_packet_t *csp_read(csp_conn_t *conn, uint32_t timeout);
void csp_reboot(uint8_t node);
csp_packet_t *csp_recvfrom(csp_socket_t *socket, uint32_t timeout);
int csp_route_start_task(unsigned int task_stack_size, unsigned int priority);
void csp_route_table_load(uint8_t *route_table_in);
void csp_route_table_save(uint8_t *route_table_out);
int csp_route_work(uint32_t timeout);
void csp_rtable_clear(void);
csp_iface_t * csp_rtable_find_iface(uint8_t id);
uint8_t csp_rtable_find_mac(uint8_t id);
void csp_rtable_print(void);
int csp_rtable_set(uint8_t node, uint8_t mask, csp_iface_t *ifc, uint8_t mac);
int csp_send(csp_conn_t *conn, csp_packet_t *packet, uint32_t timeout);
int csp_send_prio(uint8_t prio, csp_conn_t *conn, csp_packet_t *packet, uint32_t timeout);
int csp_sendto(uint8_t prio, uint8_t dest, uint8_t dport, uint8_t src_port, uint32_t opts, csp_packet_t *packet, uint32_t timeout);
int csp_sendto_reply(csp_packet_t * request_packet, csp_packet_t * reply_packet, uint32_t opts, uint32_t timeout);
void csp_service_handler(csp_conn_t *conn, csp_packet_t *packet);
void csp_set_hostname(char *hostname);
void csp_set_model(char *model);
int csp_sfp_recv(csp_conn_t * conn, void ** dataout, int * datasize, uint32_t timeout);
int csp_sfp_recv_fp(csp_conn_t * conn, void ** dataout, int * datasize, uint32_t timeout, csp_packet_t * first_packet);
int csp_sfp_send(csp_conn_t * conn, void * data, int totalsize, int mtu, uint32_t timeout);
int csp_sfp_send_own_memcpy(csp_conn_t * conn, void * data, int totalsize, int mtu, uint32_t timeout, void * (*memcpyfcn)(void *, const void *, size_t));
void csp_shutdown(uint8_t node);
csp_socket_t *csp_socket(uint32_t opts);
int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen);
int csp_transaction_persistent(csp_conn_t *conn, uint32_t timeout, void *outbuf, int outlen, void *inbuf, int inlen);
void csp_uptime(uint8_t node, uint32_t timeout);
