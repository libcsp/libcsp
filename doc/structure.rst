Structure
=========
The Cubesat Space Protocol library is structured as shown in the following table:

=============================  =========================================================================
**Folder**                     **Description**
=============================  =========================================================================
libcsp/include/csp             Main include files
libcsp/include/csp/arch        Architecture include files
libcsp/include/csp/interfaces  Interface include files
libcsp/include/csp/drivers     Drivers include files
libcsp/src                     Main modules for CSP: io, router, connections, services 
libcsp/src/interfaces          Interface modules for CAN, I2C, KISS, LOOP and ZMQHUB
libcsp/src/drivers/can         Driver for CAN                                      
libcsp/src/drivers/usart       Driver for USART                                      
libcsp/src/arch/freertos       FreeRTOS architecture module
libcsp/src/arch/macosx         Mac OS X architecture module
libcsp/src/arch/posix          Posix architecture module
libcsp/src/arch/windows        Windows architecture module
libcsp/src/rtable              Routing table module
libcsp/transport               Transport module, UDP and RDP
libcsp/crypto                  Crypto module
libcsp/utils                   Utilities
libcsp/bindings/python         Python wrapper for libcsp                                       
libcsp/examples                CSP examples (source code)                                      
libasf/doc                     The doc folder contains the source code for this documentation
=============================  =========================================================================
