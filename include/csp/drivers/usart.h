#pragma once

/**
   @file

   USART driver.

   @note This interface implementation only support ONE open UART connection.
*/

#include <csp/interfaces/csp_if_kiss.h>

#if (CSP_WINDOWS)
#include <windows.h>
#endif



/**
   OS file handle.
*/
#if (CSP_WINDOWS)
    typedef HANDLE csp_usart_fd_t;
#else
    typedef int csp_usart_fd_t;
#endif

/**
   Usart configuration.
   @see csp_usart_open()
*/
typedef struct csp_usart_conf {
    //! USART device.
    const char *device;
    //! bits per second.
    uint32_t baudrate;
    //! Number of data bits.
    uint8_t databits;
    //! Number of stop bits.
    uint8_t stopbits;
    //! Parity setting.
    uint8_t paritysetting;
    //! Enable parity checking (Windows only).
    uint8_t checkparity;
} csp_usart_conf_t;

/**
   Callback for returning data to application.

   @param[in] buf data received.
   @param[in] len data length (number of bytes in \a buf).
   @param[out] pxTaskWoken Valid reference if called from ISR, otherwise NULL!
*/
typedef void (*csp_usart_callback_t) (void * user_data, uint8_t *buf, size_t len, void *pxTaskWoken);

/**
   Opens an UART device.

   Opens the UART device and creates a thread for reading/returning data to the application.

   @note On read failure, exit() will be called - terminating the process.

   @param[in] conf UART configuration.
   @param[in] rx_callback receive data callback.
   @param[in] user_data reference forwarded to the \a rx_callback function.
   @param[out] fd the opened file descriptor.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_usart_open(const csp_usart_conf_t *conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * fd);

/**
 * Write data on open UART.
 * @return number of bytes written on success, a negative value on failure. 
 */
int csp_usart_write(csp_usart_fd_t fd, const void * data, size_t data_length);

/**
 * Lock the device, so only a single user can write to the serial port at a time
 */
void csp_usart_lock(void * driver_data);

/**
 * Unlock the USART again
 */
void csp_usart_unlock(void * driver_data);

/**
   Opens UART device and add KISS interface.

   This is a convience function for opening an UART device and adding it as an interface with a given name.

   @note On read failures, exit() will be called - terminating the process.

   @param[in] conf UART configuration.
   @param[in] ifname internface name (will be copied), or use NULL for default name.
   @param[out] return_iface the added interface.
   @return #CSP_ERR_NONE on success, otherwise an error code.
*/
int csp_usart_open_and_add_kiss_interface(const csp_usart_conf_t *conf, const char * ifname, csp_iface_t ** return_iface);
