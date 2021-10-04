# How to install LibCSP

CSP supports two build systems, the meson build system
(<https://mesonbuild.com/>) and the waf build system
(<https://waf.io/>).

## Using meson

In order to compile CSP with <span class="title-ref">meson</span>, you
run the following commands:

``` bash
meson setup builddir
cd builddir
ninja
```

You can use <span class="title-ref">meson configure</span> to change the
core options as well as compiler or project options.

## Using waf

In order to compile CSP with <span class="title-ref">waf</span>, you
first need to configure the toolchain, what operating system to compile
for, the location of required libraries and whether to enable certain
optional features.

To configure CSP to build with the AVR32 toolchain for FreeRTOS and
output the compiled libcsp.a and header files to the install directory,
issue:

``` bash
./waf configure --toolchain=avr32- --with-os=freertos --prefix=install
```

When compiling for FreeRTOS, the path to the FreeRTOS header files must
be specified with <span class="title-ref">--includes=PATH</span>.

A number of optional features can be enabled by from the configure
script. Support for XTEA encryption can e.g. be enabled with
<span class="title-ref">--enable-xtea</span>. Run
<span class="title-ref">./waf configure --help</span> to list the
available configure options.

The CAN driver (based on socketcan) can be enabled by appending the
configure option <span class="title-ref">--enable-can-socketcan</span>.

To build and copy the library to the location specified with --prefix,
use:

``` bash
./waf build install
```
