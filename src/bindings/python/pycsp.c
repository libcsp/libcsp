#include <Python.h>
#include <csp/csp.h>
#include <csp/csp_error.h>
#include <csp/csp_rtable.h>
#include <csp/csp_iflist.h>
#include <csp/csp_buffer.h>
#include <csp/csp_cmp.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>

#if PY_MAJOR_VERSION == 3
#define IS_PY3
#endif

static int is_capsule_of_type(PyObject* capsule, const char* expected_type) {
    const char* capsule_name = PyCapsule_GetName(capsule);
    if (strcmp(capsule_name, expected_type) != 0) {
        PyErr_Format(
            PyExc_TypeError,
            "capsule contains unexpected type, expected=%s, got=%s",
            expected_type, capsule_name); // TypeError is thrown
        return 0;
    }
    return 1;
}

/**
 * csp/csp.h
 */

/*
 * void csp_service_handler(csp_conn_t *conn, csp_packet_t *packet);
 */
static PyObject* pycsp_service_handler(PyObject *self, PyObject *args) {
    PyObject* conn_capsule;
    PyObject* packet_capsule;
    if (!PyArg_ParseTuple(args, "OO", &conn_capsule, &packet_capsule)) {
        return NULL; // TypeError is thrown
    }

    if (!is_capsule_of_type(conn_capsule, "csp_conn_t") ||
        !is_capsule_of_type(packet_capsule, "csp_packet_t")) {
        return NULL; // TypeError is thrown
    }

    csp_service_handler(
        (csp_conn_t*)PyCapsule_GetPointer(conn_capsule, "csp_conn_t"),
        (csp_packet_t*)PyCapsule_GetPointer(packet_capsule, "csp_packet_t"));

    Py_RETURN_NONE;
}

/*
 * int csp_init(uint8_t my_node_address);
 */
static PyObject* pycsp_init(PyObject *self, PyObject *args) {
    uint8_t my_node_address;
    if (!PyArg_ParseTuple(args, "b", &my_node_address)) {
        return NULL; // TypeError is thrown
    }

    return Py_BuildValue("i", csp_init(my_node_address));
}

/*
 * void csp_set_hostname(const char *hostname);
 */
static PyObject* pycsp_set_hostname(PyObject *self, PyObject *args) {
    char* hostname;
    if (!PyArg_ParseTuple(args, "s", &hostname)) {
        return NULL; // TypeError is thrown
    }

    csp_set_hostname(hostname);
    Py_RETURN_NONE;
}

/*
 * const char *csp_get_hostname(void);
 */
static PyObject* pycsp_get_hostname(PyObject *self, PyObject *args) {
    return Py_BuildValue("s", csp_get_hostname());
}

/*
 * void csp_set_model(const char *model);
 */
static PyObject* pycsp_set_model(PyObject *self, PyObject *args) {
    char* model;
    if (!PyArg_ParseTuple(args, "s", &model)) {
        return NULL; // TypeError is thrown
    }

    csp_set_model(model);
    Py_RETURN_NONE;
}

/*
 * const char *csp_get_model(void);
 */
static PyObject* pycsp_get_model(PyObject *self, PyObject *args) {
    return Py_BuildValue("s", csp_get_model());
}

/*
 * void csp_set_revision(const char *revision);
 */
static PyObject* pycsp_set_revision(PyObject *self, PyObject *args) {
    char* revision;
    if (!PyArg_ParseTuple(args, "s", &revision)) {
        return NULL; // TypeError is thrown
    }

    csp_set_revision(revision);
    Py_RETURN_NONE;
}

/*
 * const char *csp_get_revision(void);
 */
static PyObject* pycsp_get_revision(PyObject *self, PyObject *args) {
    return Py_BuildValue("s", csp_get_revision());
}

/*
 * csp_socket_t *csp_socket(uint32_t opts);
 */
static PyObject* pycsp_socket(PyObject *self, PyObject *args) {
    uint32_t opts = CSP_SO_NONE;
    if (!PyArg_ParseTuple(args, "|I", &opts)) {
        return NULL; // TypeError is thrown
    }

    return PyCapsule_New(csp_socket(opts), "csp_socket_t", NULL);
}

/*
 * csp_conn_t *csp_accept(csp_socket_t *socket, uint32_t timeout);
 */
static PyObject* pycsp_accept(PyObject *self, PyObject *args) {
    PyObject* socket_capsule;
    uint32_t timeout = 500;
    if (!PyArg_ParseTuple(args, "O|I", &socket_capsule, &timeout)) {
        return NULL; // TypeError is thrown
    }

    if (!is_capsule_of_type(socket_capsule, "csp_socket_t")) {
        return NULL; // TypeError is thrown
    }

    void* socket = PyCapsule_GetPointer(socket_capsule, "csp_socket_t");
    csp_conn_t* conn = csp_accept((csp_socket_t*)socket, timeout);
    if (conn == NULL) {
        Py_RETURN_NONE; // because a capsule cannot contain a NULL-pointer
    }

    return PyCapsule_New(conn, "csp_conn_t", NULL);
}

/*
 * csp_packet_t *csp_read(csp_conn_t *conn, uint32_t timeout);
 */
static PyObject* pycsp_read(PyObject *self, PyObject *args) {
    PyObject* conn_capsule;
    uint32_t timeout = 500;
    if (!PyArg_ParseTuple(args, "O|I", &conn_capsule, &timeout)) {
        return NULL; // TypeError is thrown
    }

    if (!is_capsule_of_type(conn_capsule, "csp_conn_t")) {
        return NULL; // TypeError is thrown
    }

    void* conn = PyCapsule_GetPointer(conn_capsule, "csp_conn_t");
    csp_packet_t* packet = csp_read((csp_conn_t*)conn, timeout);
    if (packet == NULL) {
        Py_RETURN_NONE; // because capsule cannot contain a NULL-pointer
    }

    return PyCapsule_New(packet, "csp_packet_t", NULL);
}

/*
 * int csp_transaction(uint8_t prio, uint8_t dest, uint8_t port,
 *                     uint32_t timeout, void *outbuf, int outlen,
 *                     void *inbuf, int inlen);
 */
static PyObject* pycsp_transaction(PyObject *self, PyObject *args) {
    uint8_t prio;
    uint8_t dest;
    uint8_t port;
    uint32_t timeout;
    Py_buffer inbuf;
    Py_buffer outbuf;
    if (!PyArg_ParseTuple(args, "bbbIw*w*", &prio, &dest, &port, &timeout, &outbuf, &inbuf)) {
        return NULL; // TypeError is thrown
    }

    int result = csp_transaction(prio, dest, port, timeout,
                                 outbuf.buf, outbuf.len,
                                 inbuf.buf, inbuf.len);

    return Py_BuildValue("i", result);
}

/*
 * int csp_sendto_reply(csp_packet_t * request_packet,
 *                      csp_packet_t * reply_packet,
 *                      uint32_t opts, uint32_t timeout);
 */
static PyObject* pycsp_sendto_reply(PyObject *self, PyObject *args) {
    PyObject* request_packet_capsule;
    PyObject* reply_packet_capsule;
    uint32_t opts = CSP_O_NONE;
    uint32_t timeout = 500;
    if (!PyArg_ParseTuple(args, "OO|II", &request_packet_capsule, &reply_packet_capsule, &opts, &timeout)) {
        return NULL; // TypeError is thrown
    }

    if (!is_capsule_of_type(request_packet_capsule, "csp_packet_t") ||
        !is_capsule_of_type(reply_packet_capsule, "csp_packet_t")) {
        return NULL; // TypeError is thrown
    }

    void* request_packet = PyCapsule_GetPointer(request_packet_capsule, "csp_packet_t");
    void* reply_packet = PyCapsule_GetPointer(reply_packet_capsule, "csp_packet_t");

    return Py_BuildValue("i", csp_sendto_reply((csp_packet_t*)request_packet,
                                               (csp_packet_t*)reply_packet,
                                               opts,
                                               timeout));
}

/*
 * int csp_close(csp_conn_t *conn);
 */
static PyObject* pycsp_close(PyObject *self, PyObject *conn_capsule) {
    if (!is_capsule_of_type(conn_capsule, "csp_conn_t")) {
        return NULL; // TypeError is thrown
    }

    void *conn = PyCapsule_GetPointer(conn_capsule, "csp_conn_t");
    return Py_BuildValue("i", csp_close((csp_conn_t*)conn));
}

/*
 * int csp_conn_dport(csp_conn_t *conn);
 */
static PyObject* pycsp_conn_dport(PyObject *self, PyObject *conn_capsule) {
    if (!is_capsule_of_type(conn_capsule, "csp_conn_t")) {
        return NULL; // TypeError is thrown
    }

    void* conn = PyCapsule_GetPointer(conn_capsule, "csp_conn_t");
    return Py_BuildValue("i", csp_conn_dport((csp_conn_t*)conn));
}

/*
 * int csp_conn_sport(csp_conn_t *conn);
 */
static PyObject* pycsp_conn_sport(PyObject *self, PyObject *conn_capsule) {
    if (!is_capsule_of_type(conn_capsule, "csp_conn_t")) {
        return NULL; // TypeError is thrown
    }

    void* conn = PyCapsule_GetPointer(conn_capsule, "csp_conn_t");
    return Py_BuildValue("i", csp_conn_sport((csp_conn_t*)conn));
}

/* int csp_conn_dst(csp_conn_t *conn); */
static PyObject* pycsp_conn_dst(PyObject *self, PyObject *conn_capsule) {
    if (!is_capsule_of_type(conn_capsule, "csp_conn_t")) {
        return NULL; // TypeError is thrown
    }

    void* conn = PyCapsule_GetPointer(conn_capsule, "csp_conn_t");
    return Py_BuildValue("i", csp_conn_dst((csp_conn_t*)conn));
}

/*
 * int csp_conn_src(csp_conn_t *conn);
 */
static PyObject* pycsp_conn_src(PyObject *self, PyObject *conn_capsule) {
    if (!is_capsule_of_type(conn_capsule, "csp_conn_t")) {
        return NULL; // TypeError is thrown
    }

    void* conn = PyCapsule_GetPointer(conn_capsule, "csp_conn_t");
    return Py_BuildValue("i", csp_conn_src((csp_conn_t*)conn));
}

/* int csp_listen(csp_socket_t *socket, size_t conn_queue_length); */
static PyObject* pycsp_listen(PyObject *self, PyObject *args) {
    PyObject* socket_capsule;
    size_t conn_queue_len = 10;
    if (!PyArg_ParseTuple(args, "O|n", &socket_capsule, &conn_queue_len)) {
        return NULL; // TypeError is thrown
    }

    if (!is_capsule_of_type(socket_capsule, "csp_socket_t")) {
        return NULL; // TypeError is thrown
    }

    void* sock = PyCapsule_GetPointer(socket_capsule, "csp_socket_t");
    return Py_BuildValue("i", csp_listen((csp_socket_t*)sock, conn_queue_len));
}

/* int csp_bind(csp_socket_t *socket, uint8_t port); */
static PyObject* pycsp_bind(PyObject *self, PyObject *args) {
    PyObject* socket_capsule;
    uint8_t port;
    if (!PyArg_ParseTuple(args, "Ob", &socket_capsule, &port)) {
        return NULL; // TypeError is thrown
    }

    if (!is_capsule_of_type(socket_capsule, "csp_socket_t")) {
        return NULL; // TypeError is thrown
    }

    void* sock = PyCapsule_GetPointer(socket_capsule, "csp_socket_t");
    return Py_BuildValue("i", csp_bind((csp_socket_t*)sock, port));
}

/* int csp_route_start_task(unsigned int task_stack_size, unsigned int priority); */
static PyObject* pycsp_route_start_task(PyObject *self, PyObject *args) {
    unsigned int priority = CSP_PRIO_NORM;
    if (!PyArg_ParseTuple(args, "|I", &priority)) {
        return NULL; // TypeError is thrown
    }

    return Py_BuildValue("i", csp_route_start_task(0, priority));
}

/*
 * int csp_ping(uint8_t node, uint32_t timeout,
 *              unsigned int size, uint8_t conn_options);
 */
static PyObject* pycsp_ping(PyObject *self, PyObject *args) {
    uint8_t node;
    uint32_t timeout = 500;
    unsigned int size = 100;
    uint8_t conn_options = CSP_O_NONE;
    if (!PyArg_ParseTuple(args, "b|IIb", &node, &timeout, &size, &conn_options)) {
        return NULL; // TypeError is thrown
    }

    return Py_BuildValue("i", csp_ping(node, timeout, size, conn_options));
}

/*
 * void csp_reboot(uint8_t node);
 */
static PyObject* pycsp_reboot(PyObject *self, PyObject *args) {
    uint8_t node;
    if (!PyArg_ParseTuple(args, "b", &node)) {
        return NULL; // TypeError is thrown
    }

    csp_reboot(node);
    Py_RETURN_NONE;
}

/*
 * void csp_shutdown(uint8_t node);
 */
static PyObject* pycsp_shutdown(PyObject *self, PyObject *args) {
    uint8_t node;
    if (!PyArg_ParseTuple(args, "b", &node)) {
        return NULL; // TypeError is thrown
    }

    csp_shutdown(node);
    Py_RETURN_NONE;
}

/*
 * void csp_rdp_set_opt(unsigned int window_size,
 *                      unsigned int conn_timeout_ms,
 *                      unsigned int packet_timeout_ms,
 *                      unsigned int delayed_acks,
 *                      unsigned int ack_timeout,
 *                      unsigned int ack_delay_count);
 */
static PyObject* pycsp_rdp_set_opt(PyObject *self, PyObject *args) {
    unsigned int window_size;
    unsigned int conn_timeout_ms;
    unsigned int packet_timeout_ms;
    unsigned int delayed_acks;
    unsigned int ack_timeout;
    unsigned int ack_delay_count;
    if (!PyArg_ParseTuple(args, "IIIIII", &window_size, &conn_timeout_ms,
                          &packet_timeout_ms, &delayed_acks,
                          &ack_timeout, &ack_delay_count)) {
        return NULL; // TypeError is thrown
    }

    csp_rdp_set_opt(window_size, conn_timeout_ms, packet_timeout_ms,
                    delayed_acks, ack_timeout, ack_delay_count);
    Py_RETURN_NONE;
}

/*
 * void csp_rdp_get_opt(unsigned int *window_size,
 *                      unsigned int *conn_timeout_ms,
 *                      unsigned int *packet_timeout_ms,
 *                      unsigned int *delayed_acks,
 *                      unsigned int *ack_timeout,
 *                      unsigned int *ack_delay_count);
 */
static PyObject* pycsp_rdp_get_opt(PyObject *self, PyObject *args) {

    unsigned int window_size = 0;
    unsigned int conn_timeout_ms = 0;
    unsigned int packet_timeout_ms = 0;
    unsigned int delayed_acks = 0;
    unsigned int ack_timeout = 0;
    unsigned int ack_delay_count = 0;
    csp_rdp_get_opt(&window_size,
                    &conn_timeout_ms,
                    &packet_timeout_ms,
                    &delayed_acks,
                    &ack_timeout,
                    &ack_delay_count);

    return Py_BuildValue("IIIIII",
                         window_size,
                         conn_timeout_ms,
                         packet_timeout_ms,
                         delayed_acks,
                         ack_timeout,
                         ack_delay_count);
}


/*
 *
 * int csp_xtea_set_key(char *key, uint32_t keylen);
 */
static PyObject* pycsp_xtea_set_key(PyObject *self, PyObject *args) {
    char* key;
    uint32_t keylen;
    if (!PyArg_ParseTuple(args, "si", &key, &keylen)) {
        return NULL; // TypeError is thrown
    }
    return Py_BuildValue("i", csp_xtea_set_key(key, keylen));
}
/**
 * csp/csp_rtable.h
 */

/*
 * int csp_rtable_set(uint8_t node, uint8_t mask,
 *                    csp_iface_t *ifc, uint8_t mac);
 */
static PyObject* pycsp_rtable_set(PyObject *self, PyObject *args) {
    uint8_t node;
    uint8_t mask;
    char* interface_name;
    uint8_t mac = CSP_NODE_MAC;
    if (!PyArg_ParseTuple(args, "bbs|b", &node, &mask, &interface_name, &mac)) {
        return NULL; // TypeError is thrown
    }

    return Py_BuildValue("i", csp_rtable_set(node,
                                             mask,
                                             csp_iflist_get_by_name(interface_name),
                                             mac));
}

/*
 * void csp_rtable_clear(void);
 */
static PyObject* pycsp_rtable_clear(PyObject *self, PyObject *args) {
    csp_rtable_clear();
    Py_RETURN_NONE;
}

/**
 * csp/csp_buffer.h
 */

/*
 * int csp_buffer_init(int count, int size);
 */
static PyObject* pycsp_buffer_init(PyObject *self, PyObject *args) {
    int count;
    int size;
    if (!PyArg_ParseTuple(args, "ii", &count, &size)) {
        return NULL; // TypeError is thrown
    }

    return Py_BuildValue("i", csp_buffer_init(count, size));
}

/*
 * void * csp_buffer_get(size_t size);
 */
static PyObject* pycsp_buffer_get(PyObject *self, PyObject *args) {
    size_t size;
    if (!PyArg_ParseTuple(args, "n", &size)) {
        return NULL; // TypeError is thrown
    }

    void* packet = csp_buffer_get(size);
    if (packet == NULL) {
        Py_RETURN_NONE;
    }

    return PyCapsule_New(packet, "csp_packet_t", NULL);
}
/*
 * void csp_buffer_free(void *packet);
 */
static PyObject* pycsp_buffer_free(PyObject *self, PyObject *args) {
    PyObject* packet_capsule;
    if (!PyArg_ParseTuple(args, "O", &packet_capsule)) {
        return NULL; // TypeError is thrown
    }


    if (!is_capsule_of_type(packet_capsule, "csp_packet_t")) {
        return NULL; // TypeError is thrown
    }

    csp_buffer_free(PyCapsule_GetPointer(packet_capsule, "csp_packet_t"));
    Py_RETURN_NONE;
}

/*
 * int csp_buffer_remaining(void);
 */
static PyObject* pycsp_buffer_remaining(PyObject *self, PyObject *args) {
    return Py_BuildValue("i", csp_buffer_remaining());
}

/**
 * csp/csp_cmp.h
 */

/*
 * static inline int csp_cmp_ident(uint8_t node, uint32_t timeout,
 *                                 struct csp_cmp_message *msg)
 */
static PyObject* pycsp_cmp_ident(PyObject *self, PyObject *args) {
    uint8_t node;
    uint32_t timeout = 500;
    if (!PyArg_ParseTuple(args, "b|i", &node, &timeout)) {
        return NULL; // TypeError is thrown
    }

    struct csp_cmp_message msg;
    int rc = csp_cmp_ident(node, timeout, &msg);
    return Py_BuildValue("isssss",
                         rc,
                         msg.ident.hostname,
                         msg.ident.model,
                         msg.ident.revision,
                         msg.ident.date,
                         msg.ident.time);
}

/*
 * static inline int csp_cmp_route_set(uint8_t node, uint32_t timeout,
 *                                 struct csp_cmp_message *msg)
 */
static PyObject* pycsp_cmp_route_set(PyObject *self, PyObject *args) {
    uint8_t node;
    uint32_t timeout = 500;
    uint8_t addr;
    uint8_t mac;
    char* ifstr;
    if (!PyArg_ParseTuple(args, "bibbs", &node, &timeout, &addr, &mac, &ifstr)) {
        return NULL; // TypeError is thrown
    }

    struct csp_cmp_message msg;
    msg.route_set.dest_node = addr;
    msg.route_set.next_hop_mac = mac;
    strncpy(msg.route_set.interface, ifstr, CSP_CMP_ROUTE_IFACE_LEN);
    int rc = csp_cmp_route_set(node, timeout, &msg);
    return Py_BuildValue("i",
                         rc);
}


/**
 * csp/interfaces/csp_if_zmqhub.h
 */

/*
 * int csp_zmqhub_init(char addr, char * host);
 */
static PyObject* pycsp_zmqhub_init(PyObject *self, PyObject *args) {
    char addr;
    char* host;
    if (!PyArg_ParseTuple(args, "bs", &addr, &host)) {
        return NULL; // TypeError is thrown
    }

    return Py_BuildValue("i", csp_zmqhub_init(addr, host));
}

/**
 * csp/drivers/can_socketcan.h
 */

/*
 * csp_iface_t * csp_can_socketcan_init(const char * ifc, int bitrate, int promisc);
 */
static PyObject* pycsp_can_socketcan_init(PyObject *self, PyObject *args)
{
    char* ifc;
    int bitrate = 1000000;
    int promisc = 0;

    if (!PyArg_ParseTuple(args, "s|ii", &ifc, &bitrate, &promisc))
    {
        return NULL;
    }

    csp_can_socketcan_init(ifc, bitrate, promisc);
    Py_RETURN_NONE;
}


/**
 * csp/interfaces/csp_if_kiss.h
 */

/*
 * int csp_kiss_init(char addr, char * host);
 */
static PyObject* pycsp_kiss_init(PyObject *self, PyObject *args) {
	char* device;
	uint32_t baudrate = 500000;
	uint32_t mtu = 512; 
	const char* if_name = "KISS";
	if (!PyArg_ParseTuple(args, "s|IIs", &device, &baudrate, &mtu, &if_name)) {
		return NULL; // TypeError is thrown
	}

	static csp_iface_t csp_if_kiss;
	static csp_kiss_handle_t csp_kiss_driver;
	csp_if_kiss.mtu = (uint16_t) mtu;
	struct usart_conf conf = {.device = device, .baudrate = baudrate};
	csp_kiss_init(&csp_if_kiss, &csp_kiss_driver, usart_putc, usart_insert, if_name);
	usart_init(&conf);
	
	void my_usart_rx(uint8_t * buf, int len, void * pxTaskWoken) {
		csp_kiss_rx(&csp_if_kiss, buf, len, pxTaskWoken);
	}
	usart_set_callback(my_usart_rx);

	Py_RETURN_NONE;
}

/**
 * Helpers - accessing csp_packet_t members
 */
static PyObject* pycsp_packet_set_data(PyObject *self, PyObject *args) {
    PyObject* packet_capsule;
    Py_buffer data;
    if (!PyArg_ParseTuple(args, "Ow*", &packet_capsule, &data)) {
        return NULL; // TypeError is thrown
    }

    if (!is_capsule_of_type(packet_capsule, "csp_packet_t")) {
        return NULL; // TypeError is thrown
    }

    csp_packet_t* packet = PyCapsule_GetPointer(packet_capsule, "csp_packet_t");

    strncpy((char *)packet->data, data.buf, data.len);
    packet->length = data.len;

    Py_RETURN_NONE;
}
static PyObject* pycsp_packet_get_data(PyObject *self, PyObject *packet_capsule) {
    if (!is_capsule_of_type(packet_capsule, "csp_packet_t")) {
        return NULL; // TypeError is thrown
    }

    csp_packet_t* packet = PyCapsule_GetPointer(packet_capsule, "csp_packet_t");
#ifdef IS_PY3
    return Py_BuildValue("y#", packet->data, packet->length);
#else
    return Py_BuildValue("s#", packet->data, packet->length);
#endif
}

static PyObject* pycsp_packet_get_length(PyObject *self, PyObject *packet_capsule) {
    if (!is_capsule_of_type(packet_capsule, "csp_packet_t")) {
        return NULL; // TypeError is thrown
    }

    csp_packet_t* packet = PyCapsule_GetPointer(packet_capsule, "csp_packet_t");
    return Py_BuildValue("H", packet->length);
}

static PyMethodDef methods[] = {

    /* csp/csp.h */
    {"service_handler", pycsp_service_handler, METH_VARARGS, ""},
    {"init", pycsp_init, METH_VARARGS, ""},
    {"set_hostname", pycsp_set_hostname, METH_VARARGS, ""},
    {"get_hostname", pycsp_get_hostname, METH_NOARGS, ""},
    {"set_model", pycsp_set_model, METH_VARARGS, ""},
    {"get_model", pycsp_get_model, METH_NOARGS, ""},
    {"set_revision", pycsp_set_revision, METH_VARARGS, ""},
    {"get_revision", pycsp_get_revision, METH_NOARGS, ""},
    {"socket", pycsp_socket, METH_VARARGS, ""},
    {"accept", pycsp_accept, METH_VARARGS, ""},
    {"read", pycsp_read, METH_VARARGS, ""},
    {"transaction", pycsp_transaction, METH_VARARGS, ""},
    {"sendto_reply", pycsp_sendto_reply, METH_VARARGS, ""},
    {"close", pycsp_close, METH_O, ""},
    {"conn_dport", pycsp_conn_dport, METH_O, ""},
    {"conn_sport", pycsp_conn_sport, METH_O, ""},
    {"conn_dst", pycsp_conn_dst, METH_O, ""},
    {"conn_src", pycsp_conn_src, METH_O, ""},
    {"listen", pycsp_listen, METH_VARARGS, ""},
    {"bind", pycsp_bind, METH_VARARGS, ""},
    {"route_start_task", pycsp_route_start_task, METH_VARARGS, ""},
    {"ping", pycsp_ping, METH_VARARGS, ""},
    {"reboot", pycsp_reboot, METH_VARARGS, ""},
    {"shutdown", pycsp_shutdown, METH_VARARGS, ""},
    {"rdp_set_opt", pycsp_rdp_set_opt, METH_VARARGS, ""},
    {"rdp_get_opt", pycsp_rdp_get_opt, METH_NOARGS, ""},
    {"xtea_set_key", pycsp_xtea_set_key, METH_VARARGS, ""},

    /* csp/csp_rtable.h */
    {"rtable_set", pycsp_rtable_set, METH_VARARGS, ""},
    {"rtable_clear", pycsp_rtable_clear, METH_NOARGS, ""},

    /* csp/csp_buffer.h */
    {"buffer_init", pycsp_buffer_init, METH_VARARGS, ""},
    {"buffer_free", pycsp_buffer_free, METH_VARARGS, ""},
    {"buffer_get", pycsp_buffer_get, METH_VARARGS, ""},
    {"buffer_remaining", pycsp_buffer_remaining, METH_NOARGS, ""},

    /* csp/csp_cmp.h */
    {"cmp_ident", pycsp_cmp_ident, METH_VARARGS, ""},
    {"cmp_route_set", pycsp_cmp_route_set, METH_VARARGS, ""},

    /* csp/interfaces/csp_if_zmqhub.h */
    {"zmqhub_init", pycsp_zmqhub_init, METH_VARARGS, ""},
    {"kiss_init", pycsp_kiss_init, METH_VARARGS, ""},

    /* csp/drivers/can_socketcan.h */
    {"can_socketcan_init", pycsp_can_socketcan_init, METH_VARARGS, ""},

    /* helpers */
    {"packet_get_length", pycsp_packet_get_length, METH_O, ""},
    {"packet_get_data", pycsp_packet_get_data, METH_O, ""},
    {"packet_set_data", pycsp_packet_set_data, METH_VARARGS, ""},

    /* sentinel */
    {NULL, NULL, 0, NULL}
};

#ifdef IS_PY3
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "libcsp_py3",
    NULL,
    -1,
    methods,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif

#ifdef IS_PY3
PyMODINIT_FUNC PyInit_libcsp_py3(void) {
#else
    PyMODINIT_FUNC initlibcsp_py2(void) {
#endif

        PyObject* m;

#ifdef IS_PY3
        m = PyModule_Create(&moduledef);
#else
        m = Py_InitModule("libcsp_py2", methods);
#endif
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
        PyModule_AddIntConstant(m, "CSP_SO_NONE", CSP_SO_NONE);
        PyModule_AddIntConstant(m, "CSP_SO_RDPREQ", CSP_SO_RDPREQ);
        PyModule_AddIntConstant(m, "CSP_SO_RDPPROHIB", CSP_SO_RDPPROHIB);
        PyModule_AddIntConstant(m, "CSP_SO_HMACREQ", CSP_SO_HMACREQ);
        PyModule_AddIntConstant(m, "CSP_SO_HMACPROHIB", CSP_SO_HMACPROHIB);
        PyModule_AddIntConstant(m, "CSP_SO_XTEAREQ", CSP_SO_XTEAREQ);
        PyModule_AddIntConstant(m, "CSP_SO_XTEAPROHIB", CSP_SO_XTEAPROHIB);
        PyModule_AddIntConstant(m, "CSP_SO_CRC32REQ", CSP_SO_CRC32REQ);
        PyModule_AddIntConstant(m, "CSP_SO_CRC32PROHIB", CSP_SO_CRC32PROHIB);
        PyModule_AddIntConstant(m, "CSP_SO_CONN_LESS", CSP_SO_CONN_LESS);

        /* CONNECT OPTIONS */
        PyModule_AddIntConstant(m, "CSP_O_NONE", CSP_O_NONE);
        PyModule_AddIntConstant(m, "CSP_O_RDP", CSP_O_RDP);
        PyModule_AddIntConstant(m, "CSP_O_NORDP", CSP_O_NORDP);
        PyModule_AddIntConstant(m, "CSP_O_HMAC", CSP_O_HMAC);
        PyModule_AddIntConstant(m, "CSP_O_NOHMAC", CSP_O_NOHMAC);
        PyModule_AddIntConstant(m, "CSP_O_XTEA", CSP_O_XTEA);
        PyModule_AddIntConstant(m, "CSP_O_NOXTEA", CSP_O_NOXTEA);
        PyModule_AddIntConstant(m, "CSP_O_CRC32", CSP_O_CRC32);
        PyModule_AddIntConstant(m, "CSP_O_NOCRC32", CSP_O_NOCRC32);


        /**
         * csp/csp_error.h
         */

        PyModule_AddIntConstant(m, "CSP_ERR_NONE", CSP_ERR_NONE);
        PyModule_AddIntConstant(m, "CSP_ERR_NOMEM", CSP_ERR_NOMEM);
        PyModule_AddIntConstant(m, "CSP_ERR_INVAL", CSP_ERR_INVAL);
        PyModule_AddIntConstant(m, "CSP_ERR_TIMEDOUT", CSP_ERR_TIMEDOUT);
        PyModule_AddIntConstant(m, "CSP_ERR_USED", CSP_ERR_USED);
        PyModule_AddIntConstant(m, "CSP_ERR_NOTSUP", CSP_ERR_NOTSUP);
        PyModule_AddIntConstant(m, "CSP_ERR_BUSY", CSP_ERR_BUSY);
        PyModule_AddIntConstant(m, "CSP_ERR_ALREADY", CSP_ERR_ALREADY);
        PyModule_AddIntConstant(m, "CSP_ERR_RESET", CSP_ERR_RESET);
        PyModule_AddIntConstant(m, "CSP_ERR_NOBUFS", CSP_ERR_NOBUFS);
        PyModule_AddIntConstant(m, "CSP_ERR_TX", CSP_ERR_TX);
        PyModule_AddIntConstant(m, "CSP_ERR_DRIVER", CSP_ERR_DRIVER);
        PyModule_AddIntConstant(m, "CSP_ERR_AGAIN", CSP_ERR_AGAIN);
        PyModule_AddIntConstant(m, "CSP_ERR_HMAC", CSP_ERR_HMAC);
        PyModule_AddIntConstant(m, "CSP_ERR_XTEA", CSP_ERR_XTEA);
        PyModule_AddIntConstant(m, "CSP_ERR_CRC32", CSP_ERR_CRC32);

        /**
         * csp/rtable.h
         */
        PyModule_AddIntConstant(m, "CSP_NODE_MAC", CSP_NODE_MAC);

#ifdef IS_PY3
        return m;
#endif
    }

