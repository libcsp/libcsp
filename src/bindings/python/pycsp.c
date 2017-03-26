#include <Python.h>
#include <csp/csp.h>
#include <csp/interfaces/csp_if_zmqhub.h>

/**
 * csp/csp.h
 */

/* int csp_init(uint8_t my_node_address); */
static PyObject* pycsp_csp_init(PyObject *self, PyObject *args) {
    uint8_t node;
    if (!PyArg_ParseTuple(args, "b", &node))
        return NULL;

    return Py_BuildValue("i", csp_init(node));
}

/*
void csp_set_address(uint8_t addr);
uint8_t csp_get_address(void);
void csp_set_hostname(char *hostname);
char *csp_get_hostname(void);
void csp_set_model(char *model);
char *csp_get_model(void);
void csp_set_revision(char *revision);
char *csp_get_revision(void);
*/

/* csp_socket_t *csp_socket(uint32_t opts); */
static PyObject* pycsp_csp_socket(PyObject *self, PyObject *args) {
    uint32_t opts = CSP_SO_NONE;
    if (!PyArg_ParseTuple(args, "|I", &opts))
        return NULL;

    return PyCapsule_New(csp_socket(opts), "csp_socket_t", NULL);
}

/* csp_conn_t *csp_accept(csp_socket_t *socket, uint32_t timeout); */
static PyObject* pycsp_csp_accept(PyObject *self, PyObject *args) {
    PyObject* sock_capsule;
    uint32_t timeout;
    if (!PyArg_ParseTuple(args, "OI", &sock_capsule, &timeout))
        return NULL;

    csp_conn_t* conn = csp_accept(PyCapsule_GetPointer(sock_capsule, "csp_socket_t"), timeout);
    if (conn == NULL)
        Py_RETURN_NONE;
    return PyCapsule_New(conn, "csp_conn_t", NULL);
}

/* csp_packet_t *csp_read(csp_conn_t *conn, uint32_t timeout); */
static PyObject* pycsp_csp_read(PyObject *self, PyObject *args) {
    PyObject* conn_capsule;
    uint32_t timeout;
    if (!PyArg_ParseTuple(args, "OI", &conn_capsule, &timeout))
        return NULL;

    csp_packet_t* packet = csp_read(PyCapsule_GetPointer(conn_capsule, "csp_conn_t"), timeout);
    if (packet == NULL)
        Py_RETURN_NONE;
    return PyCapsule_New(packet, "csp_packet_t", NULL);
}

/*
int csp_send(csp_conn_t *conn, csp_packet_t *packet, uint32_t timeout);
int csp_send_prio(uint8_t prio, csp_conn_t *conn, csp_packet_t *packet, uint32_t timeout);
int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port, uint32_t timeout,
                    void *outbuf, int outlen, void *inbuf, int inlen);
int csp_transaction_persistent(csp_conn_t *conn, uint32_t timeout, void *outbuf,
                               int outlen, void *inbuf, int inlen);
csp_packet_t *csp_recvfrom(csp_socket_t *socket, uint32_t timeout);
int csp_sendto(uint8_t prio, uint8_t dest, uint8_t dport, uint8_t src_port,
               uint32_t opts, csp_packet_t *packet, uint32_t timeout);
int csp_sendto_reply(csp_packet_t * request_packet, csp_packet_t * reply_packet, uint32_t opts, uint32_t timeout);
csp_conn_t *csp_connect(uint8_t prio, uint8_t dest, uint8_t dport, uint32_t timeout, uint32_t opts);
*/

/* int csp_close(csp_conn_t *conn); */
static PyObject* pycsp_csp_close(PyObject *self, PyObject *conn_capsule) {
    return Py_BuildValue("i", csp_close(PyCapsule_GetPointer(conn_capsule, "csp_conn_t")));
}

/* int csp_conn_dport(csp_conn_t *conn); */
static PyObject* pycsp_csp_conn_dport(PyObject *self, PyObject *conn_capsule) {
    return Py_BuildValue("i", csp_conn_dport(PyCapsule_GetPointer(conn_capsule, "csp_conn_t")));
}

/*
int csp_conn_sport(csp_conn_t *conn);
int csp_conn_dst(csp_conn_t *conn);
int csp_conn_src(csp_conn_t *conn);
int csp_conn_flags(csp_conn_t *conn);
*/

/* int csp_listen(csp_socket_t *socket, size_t conn_queue_length); */
static PyObject* pycsp_csp_listen(PyObject *self, PyObject *args) {
    PyObject* sock_capsule;
    size_t conn_queue_len;
    if (!PyArg_ParseTuple(args, "On", &sock_capsule, &conn_queue_len))
        return NULL;

    return Py_BuildValue("i", csp_listen(PyCapsule_GetPointer(sock_capsule, "csp_socket_t"), conn_queue_len));
}

/* int csp_bind(csp_socket_t *socket, uint8_t port); */
static PyObject* pycsp_csp_bind(PyObject *self, PyObject *args) {
    PyObject* sock_capsule;
    uint8_t port;
    if (!PyArg_ParseTuple(args, "Ob", &sock_capsule, &port))
        return NULL;

    return Py_BuildValue("i", csp_bind(PyCapsule_GetPointer(sock_capsule, "csp_socket_t"), port));
}

/* int csp_route_start_task(unsigned int task_stack_size, unsigned int priority);*/
static PyObject* pycsp_csp_route_start_task(PyObject *self, PyObject *args) {
    unsigned int task_stack_size;
    unsigned int priority;
    if (!PyArg_ParseTuple(args, "II", &task_stack_size, &priority))
        return NULL;

    return Py_BuildValue("i", csp_route_start_task(task_stack_size, priority));
}

/*
int csp_route_work(uint32_t timeout);
int csp_bridge_start(unsigned int task_stack_size, unsigned int task_priority,
                     csp_iface_t * _if_a, csp_iface_t * _if_b);
int csp_promisc_enable(unsigned int buf_size);
void csp_promisc_disable(void);
csp_packet_t *csp_promisc_read(uint32_t timeout);
int csp_sfp_send(csp_conn_t * conn, void * data, int totalsize, int mtu, uint32_t timeout);
int csp_sfp_send_own_memcpy(csp_conn_t * conn, void * data, int totalsize,
                            int mtu, uint32_t timeout, void * (*memcpyfcn)(void *, const void *, size_t));
int csp_sfp_recv(csp_conn_t * conn, void ** dataout, int * datasize, uint32_t timeout);
int csp_sfp_recv_fp(csp_conn_t * conn, void ** dataout, int * datasize, uint32_t timeout, csp_packet_t * first_packet);
void csp_service_handler(csp_conn_t *conn, csp_packet_t *packet);
*/

/* int csp_ping(uint8_t node, uint32_t timeout, unsigned int size, uint8_t conn_options);*/
static PyObject* pycsp_csp_ping(PyObject *self, PyObject *args) {
    uint8_t node;
    uint32_t timeout = 1000;
    unsigned int size = 100;
    uint8_t conn_options = CSP_O_NONE;

    if (!PyArg_ParseTuple(args, "b|IIb", &node, &timeout, &size, &conn_options))
        return NULL;

    printf("pinging %i\n", node);
    
    return Py_BuildValue("i", csp_ping(node, timeout, size, conn_options));
}

/*
void csp_ping_noreply(uint8_t node);
void csp_ps(uint8_t node, uint32_t timeout);
void csp_memfree(uint8_t node, uint32_t timeout);
void csp_buf_free(uint8_t node, uint32_t timeout);
void csp_reboot(uint8_t node);
void csp_shutdown(uint8_t node);
void csp_uptime(uint8_t node, uint32_t timeout);
void csp_rdp_set_opt(unsigned int window_size, unsigned int conn_timeout_ms,
                     unsigned int packet_timeout_ms, unsigned int delayed_acks,
                     unsigned int ack_timeout, unsigned int ack_delay_count);
void csp_rdp_get_opt(unsigned int *window_size, unsigned int *conn_timeout_ms,
                     unsigned int *packet_timeout_ms, unsigned int *delayed_acks,
                     unsigned int *ack_timeout, unsigned int *ack_delay_count);
int csp_xtea_set_key(char *key, uint32_t keylen);
int csp_hmac_set_key(char *key, uint32_t keylen);
void csp_conn_print_table(void);
void csp_buffer_print_table(void);
*/


/**
 * csp/csp_rtable.h
 */

/*
csp_iface_t * csp_rtable_find_iface(uint8_t id);
uint8_t csp_rtable_find_mac(uint8_t id);
*/

/* int csp_rtable_set(uint8_t node, uint8_t mask, csp_iface_t *ifc, uint8_t mac);*/
static PyObject* pycsp_csp_rtable_set(PyObject *self, PyObject *args) {
    uint8_t node;
    uint8_t mask;
    PyObject* ifc_capsule;
    uint8_t mac;
    if (!PyArg_ParseTuple(args, "bbOb", &node, &mask, &ifc_capsule, &mac))
        return NULL;

    return Py_BuildValue("i", csp_rtable_set(node, mask,
                                             PyCapsule_GetPointer(ifc_capsule, "csp_iface_t"),
                                             mac));
}

/* void csp_rtable_print(void); */
static PyObject* pycsp_csp_rtable_print(PyObject *self, PyObject *args) {
    csp_rtable_print();
    Py_RETURN_NONE;
}

/*
int csp_rtable_save(char * buffer, int maxlen);
void csp_rtable_load(char * buffer);
int csp_rtable_check(char * buffer);
void csp_rtable_clear(void);
*/


/**
 * csp/csp_buffer.h
 */

/* int csp_buffer_init(int count, int size); */
static PyObject* pycsp_csp_buffer_init(PyObject *self, PyObject *args) {
    int count;
    int size;
    if (!PyArg_ParseTuple(args, "ii", &count, &size))
        return NULL;
    return Py_BuildValue("i", csp_buffer_init(count, size));
}

/*
void * csp_buffer_get(size_t size);
void * csp_buffer_get_isr(size_t buf_size);
*/

/* void csp_buffer_free(void *packet); */
static PyObject* pycsp_csp_buffer_free(PyObject *self, PyObject *packet_capsule) {
    csp_buffer_free(PyCapsule_GetPointer(packet_capsule, "csp_packet_t"));
    Py_RETURN_NONE;
}

/*
void csp_buffer_free_isr(void *packet);
void * csp_buffer_clone(void *buffer);
int csp_buffer_remaining(void);
int csp_buffer_size(void);
*/

/**
 * csp/interfaces/csp_if_zmqhub.h
 */

/*int csp_zmqhub_init(char addr, char * host);*/
static PyObject* pycspzmq_csp_zmqhub_init(PyObject *self, PyObject *args) {
    char addr;
    char* host;
    if (!PyArg_ParseTuple(args, "bs", &addr, &host))
        return NULL;

    return Py_BuildValue("i", csp_zmqhub_init(addr, host));
}

/*
int csp_zmqhub_init_w_endpoints(char _addr, char * publisher_url, char * subscriber_url);
*/


/**
 * Helpers - accessing csp_packet_t members
 */
static PyObject* pycsp_packet_data(PyObject *self, PyObject *packet_capsule) {
    csp_packet_t* packet = PyCapsule_GetPointer(packet_capsule, "csp_packet_t");
    return Py_BuildValue("s#", packet->data, packet->length);
}

static PyObject* pycsp_packet_length(PyObject *self, PyObject *packet_capsule) {
    csp_packet_t* packet = PyCapsule_GetPointer(packet_capsule, "csp_packet_t");
    return Py_BuildValue("H", packet->length);
}

/**
 * Helpers - return csp_iface_t's as capsules
 */
static PyObject* pycspzmq_csp_zmqhub_if(PyObject *self, PyObject *args) {
    return PyCapsule_New(&csp_if_zmqhub, "csp_iface_t", NULL);
}

static PyMethodDef pycsp_methods[] = {

    /* csp/csp.h */
    {"csp_init", pycsp_csp_init, METH_VARARGS, ""},
    {"csp_socket", pycsp_csp_socket, METH_VARARGS, ""},
    {"csp_accept", pycsp_csp_accept, METH_VARARGS, ""},
    {"csp_read", pycsp_csp_read, METH_VARARGS, ""},
    {"csp_close", pycsp_csp_close, METH_O, ""},
    {"csp_conn_dport", pycsp_csp_conn_dport, METH_O, ""},
    {"csp_listen", pycsp_csp_listen, METH_VARARGS, ""},
    {"csp_bind", pycsp_csp_bind, METH_VARARGS, ""},
    {"csp_route_start_task", pycsp_csp_route_start_task, METH_VARARGS, ""},
    {"csp_ping", pycsp_csp_ping, METH_VARARGS, ""},

    /* csp/csp_rtable.h */
    {"csp_rtable_set", pycsp_csp_rtable_set, METH_VARARGS, ""},
    {"csp_rtable_print", pycsp_csp_rtable_print, METH_NOARGS, ""},

   /* csp/csp_buffer.h */
    {"csp_buffer_init", pycsp_csp_buffer_init, METH_VARARGS, ""},
    {"csp_buffer_free", pycsp_csp_buffer_free, METH_O, ""},

    /* csp/interfaces/csp_if_zmqhub.h */
    {"csp_zmqhub_init", pycspzmq_csp_zmqhub_init, METH_VARARGS, ""},

    /* helpers */
    {"csp_zmqhub_if", pycspzmq_csp_zmqhub_if, METH_NOARGS, ""},
    {"packet_length", pycsp_packet_length, METH_O, ""},
    {"packet_data", pycsp_packet_data, METH_O, ""},

    /* sentinel */
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initlibcsp(void) {
	PyObject* m = Py_InitModule("libcsp", pycsp_methods);

        /**
         * csp/csp_types.h
         */

        /* RESERVED PORTS */
        PyModule_AddIntConstant(m, "CSP_CMP", CSP_CMP);
        PyModule_AddIntConstant(m, "CSP_PING", CSP_PING);
        PyModule_AddIntConstant(m, "CSP_PS", CSP_PS);
        PyModule_AddIntConstant(m, "CSP_MEMFREE", CSP_MEMFREE);
        PyModule_AddIntConstant(m, "CSP_REBOOT", CSP_REBOOT);
        PyModule_AddIntConstant(m, "CSP_BUF_FREE", CSP_BUF_FREE);
        PyModule_AddIntConstant(m, "CSP_UPTIME", CSP_UPTIME);
        PyModule_AddIntConstant(m, "CSP_ANY", CSP_MAX_BIND_PORT + 1);
        PyModule_AddIntConstant(m, "CSP_PROMISC", CSP_MAX_BIND_PORT + 2);

        /* PRIORITIES */
        PyModule_AddIntConstant(m, "CSP_PRIO_CRITICAL", CSP_PRIO_CRITICAL);
        PyModule_AddIntConstant(m, "CSP_PRIO_HIGH", CSP_PRIO_HIGH);
        PyModule_AddIntConstant(m, "CSP_PRIO_NORM", CSP_PRIO_NORM);
        PyModule_AddIntConstant(m, "CSP_PRIO_LOW", CSP_PRIO_LOW);

        /* FLAGS */
        PyModule_AddIntConstant(m, "CSP_FFRAG", CSP_FFRAG);
        PyModule_AddIntConstant(m, "CSP_FHMAC", CSP_FHMAC);
        PyModule_AddIntConstant(m, "CSP_FXTEA", CSP_FXTEA);
        PyModule_AddIntConstant(m, "CSP_FRDP", CSP_FRDP);
        PyModule_AddIntConstant(m, "CSP_FCRC32", CSP_FCRC32);

        /* SOCKET OPTIONS */

        /* CONNECT OPTIONS */

        /**
         * csp/rtable.h
         */
        PyModule_AddIntConstant(m, "CSP_NODE_MAC", CSP_NODE_MAC);
}

