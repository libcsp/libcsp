# Structure

The Cubesat Space Protocol library is structured as shown in the
following table:

| **Folder**                      | **Description**                                     |
|---------------------------------|-----------------------------------------------------|
| `include/csp`                   | Public header files                                 |
| `include/csp/arch`              | Architecture (platform)                             |
| `include/csp/crypto`            | HMAC, SHA                                           |
| `include/csp/interfaces`        | Interfaces                                          |
| `include/csp/drivers`           | Drivers                                             |
| `src`                           | Source modules, internal headers, routing tables, transport layer |
| `src/arch`                      | Architecture (platform) specific code               |
| `src/arch/freertos`             | FreeRTOS                                            |
| `src/arch/posix`                | POSIX (Linux)                                       |
| `src/arch/windows`              | Windows                                             |
| `src/arch/std`                  | Cross-platform standard utilities (endianness)      |
| `src/arch/zephyr`               | Zephyr RTOS                                         |
| `src/bindings/python`           | Python3 wrapper for libcsp                          |
| `src/crypto`                    | HMAC, SHA                                           |
| `src/drivers`                   | Drivers, mostly platform specific                   |
| `src/drivers/can/linux/socketcan` | Linux SocketCAN driver                            |
| `src/drivers/can/zephyr`        | Zephyr CAN driver                                   |
| `src/drivers/usart`             | USART                                               |
| `src/interfaces`                | Interfaces: CAN, ETH, I2C, KISS, LOOPBACK, RF, TUN, UDP, ZMQHUB |
| `contrib`                       | Community contributions: drivers, scripts, toolchains, Zephyr module |
| `samples`                       | POSIX sample applications                           |
| `unittests`                     | Unit tests                                          |
| `utils`                         | Utilities, Python scripts for decoding CSP headers  |
| `examples`                      | CSP examples, C/Python, zmqproxy                    |
| `doc`                           | Markdown/RST documentation                          |
| `ci`                            | CI infrastructure (Dockerfile, build scripts)       |
| `cmake`                         | CMake helper modules                                |
| `csp_virtual_topology`          | Virtual CSP network topology simulator              |
