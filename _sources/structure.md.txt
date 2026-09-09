# Structure

The Cubesat Space Protocol library is structured as shown in the
following table:

| **Folder**                      | **Description**                                     |
|---------------------------------|-----------------------------------------------------|
| `libcsp/include/csp`            | Public header files                                 |
| `libcsp/include/csp/arch`       | Architecture (platform)                             |
| `libcsp/include/csp/crypto`     | HMAC, SHA                                           |
| `libcsp/include/csp/interfaces` | Interfaces                                          |
| `libcsp/include/csp/drivers`    | Drivers                                             |
| `libcsp/src`                    | Source modules and internal header files            |
| `libcsp/src/arch`               | Architecture (platform) specific code               |
| `libcsp/src/arch/freertos`      | FreeRTOS                                            |
| `libcsp/src/arch/posix`         | Posix (Linux)                                       |
| `libcsp/src/arch/zephyr`        | Zephyr                                              |
| `libcsp/src/bindings/python`    | Python3 wrapper for libcsp                          |
| `libcsp/src/crypto`             | HMAC, SHA                                           |
| `libcsp/src/drivers`            | Drivers, mostly platform specific (Linux)           |
| `libcsp/src/drivers/can`        | CAN                                                 |
| `libcsp/src/drivers/usart`      | USART                                               |
| `libcsp/src/drivers/eth`        | ETH                                                 |
| `libcsp/src/interfaces`         | Interfaces, CAN, I2C, KISS, LOOPBACK, ZMQHUB and others    |
| `libcsp/utils`                  | Utilities, Python scripts for decoding CSP headers. |
| `libcsp/examples`               | CSP examples, C/Python, zmqproxy                    |
| `libcsp/doc`                    | RST based documentation (this documentation)        |
| `libcsp/samples`                | Simple sample                                       |
| `libcsp/samples/posix`          | Posix simple sample                                 |
| `libcsp/unittests`              | Unittests                                           |
| `libcsp/contrib`                | Community contributions                             |
| `libcsp/contrib/macosx`         | MacOSx                                              |
| `libcsp/contrib/windows`        | Windows                                             |
| `libcsp/contrib/zephyr`         | Zephyr samples                                      |
