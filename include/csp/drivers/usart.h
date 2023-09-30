/****************************************************************************
 * **File:** drivers/usart.h
 *
 * **Description:** USART driver
 *
 * .. note:: This interface implementation only support ONE open UART connection.
 ****************************************************************************/

#pragma once

#include <csp/interfaces/csp_if_kiss.h>

#if (CSP_WINDOWS)
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * OS file handle.
 */
#if (CSP_WINDOWS)
    typedef HANDLE csp_usart_fd_t;
#else
    typedef int csp_usart_fd_t;
#endif

/**
 * Usart configuration.
 * @see csp_usart_open()
 */
typedef struct csp_usart_conf {
    const char *device; /**< USART device.*/
    uint32_t baudrate; /**< bits per second. */
    uint8_t databits; /**< Number of data bits. */
    uint8_t stopbits; /**< Number of stop bits. */
    uint8_t paritysetting; /**< Parity setting. */
    uint8_t checkparity; /**< Enable parity checking (Windows only). */
} csp_usart_conf_t;

/**
 * Callback for returning data to application.
 *
 * Parameters:
 *	user_data (void *) [in]: User data
 *	buf (uint8_t *) [in]: data received.
 *	len (size_t) [in]: data length (number of bytes in \a buf).
 *	pxTaskWoken (void *) [out]: Valid reference if called from ISR, otherwise NULL!
 */
typedef void (*csp_usart_callback_t) (void * user_data, uint8_t *buf, size_t len, void *pxTaskWoken);

/**
 * Opens an UART device.
 * Opens the UART device and creates a thread for reading/returning data to the application.
 *
 * .. note:: On read failure, exit() will be called - terminating the process.
 *
 * Parameters:
 *	conf (const csp_usart_conf_t *) [in]: UART configuration.
 *	rx_callback (csp_usart_callback_t) [in]: receive data callback.
 *	user_data (void *) [in]: reference forwarded to the rx_callback function.
 *	fd (csp_usart_fd_t *) [out]: the opened file descriptor.
 *
 * Returns:
 *	int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_usart_open(const csp_usart_conf_t *conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * fd);

/**
 * Write data on open UART.
 *
 * Parameters:
 *	data (const void *) [in]: Outbound data to send over the USART interface.
 *	data_length (size_t) [in]: Outbound data length.
 *
 * Returns:
 *	int: number of bytes written on success, a negative value on failure.
 */
int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length);

/**
 * Lock the device, so only a single user can write to the serial port at a time
 *
 * Parameters:
 *	driver_data (void *) [in]: Driver data
 */
void csp_usart_lock(void * driver_data);

/**
 * Unlock the USART again
 *
 * Parameters:
 *	driver_data (void *) [in]: Driver data
 */
void csp_usart_unlock(void * driver_data);

/**
 * Opens UART device and add KISS interface.
 *
 * This is a convience function for opening an UART device and adding it as an interface with a given name.
 *
 * .. note:: On read failures, exit() will be called - terminating the process.
 *
 * Parameters:
 *	conf (const csp_usart_conf_t *) [in]: UART configuration.
 *	ifname (const char *) [in]: internface name (will be copied), or use NULL for default name.
 *	return_iface (csp_iface_t **) [out]: the added interface.
 *
 * Returns:
 *	int: #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_usart_open_and_add_kiss_interface(const csp_usart_conf_t *conf, const char * ifname, csp_iface_t ** return_iface);

#ifdef __cplusplus
}
#endif
