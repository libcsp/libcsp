How to install LibCSP
=====================

CSP uses the `waf` build system (http://code.google.com/p/waf/). In order to
compile CSP, you first need to configure the toolchain, what operating system
to compile for, the location of required libraries and whether to enable 
certain optional features.

To configure CSP to build with the AVR32 toolchain for FreeRTOS and output
the compiled libcsp.a and header files to the install directory, issue:

.. code-block:: bash

   ./waf configure --toolchain=avr32- --with-os=freertos --prefix=install

When compiling for FreeRTOS, the path to the FreeRTOS header files must be
specified with `--includes=PATH`.

A number of optional features can be enabled by from the configure script.
Support for XTEA encryption can e.g. be enabled with `--enable-xtea`. Run 
`./waf configure --help` to list the available configure options.

The CAN driver (based on socketcan) can be enabled by appending the configure option `--enable-can-socketcan`. 

To build and copy the library to the location specified with --prefix, use:

.. code-block:: bash

   ./waf build install
