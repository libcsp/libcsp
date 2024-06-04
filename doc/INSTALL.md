# How to install LibCSP

```{contents}
:depth: 3
```

CSP supports three build systems:

- [the Meson build system](https://mesonbuild.com/)
- [the Waf build system](https://waf.io/)
- [the CMake build system](https://cmake.org/)

## Using Meson

In order to compile CSP with `meson`, you
run the following commands:

```shell
meson setup builddir
cd builddir
ninja
```

You can use `meson configure` to change the
core options as well as compiler or project options.

## Using Waf

In order to compile CSP with `waf`, you
first need to configure the toolchain, what operating system to compile
for, the location of required libraries and whether to enable certain
optional features.

To configure CSP to build with the AVR32 toolchain for FreeRTOS and
output the compiled libcsp.a and header files to the install directory,
issue:

```shell
./waf configure --toolchain=avr32- --with-os=freertos --prefix=install
```

When compiling for FreeRTOS, the path to the FreeRTOS header files must
be specified with `--includes=PATH`.

A number of optional features can be enabled by from the configure
script.
`./waf configure --help` to list the
available configure options.

The CAN driver (based on socketcan) can be enabled by appending the
configure option `--enable-can-socketcan`.

To build and copy the library to the location specified with --prefix,
use:

```shell
./waf build install
```

## Using CMake

Make sure [Ninja](https://ninja-build.org/) is installed on your system first.
You can now run the following commands to compile CSP with `cmake`:

```shell
cmake -G Ninja -B builddir
cmake --build builddir
```

Please note that other build system generators might work as well, but `Ninja` is the officially
supported and tested build system when using CMake.
To install the compiled libcsp.so and header files to the install directory,
you run the following command:

```shell
cmake --install builddir
```

Please note that `sudo` might be required to install files into the default install directories.
By default, it will be installed in `/usr/local/lib` and `/usr/local/include`,
but if you wish to change it, you can specify `-DCMAKE_INSTALL_PREFIX=<path>`
during the build process, and it will be installed in `<path>/lib` and `<path>/include`.

To install only the libcsp.so runtime library,
use the following command:

```shell
cmake --install builddir --component runtime
```

## Reproducible Builds

libcsp supports Reproducible Builds. To enable it, set
`CSP_REPRODUCIBLE_BUILDS` to `1`.

Please note that, when reproducible builds are enabled,
`CSP_CMP_IDENT` does not return the compilation date and time.

When reproducible builds are enabled, both the BuildID and hash values
of generated binaries remain consistent for each build. Our
reproducible builds also support building in different directories.
Thus, different users should generate precisely the same binaries,
given the same source code and build environment.

You can learn more about reproducible builds at
https://reproducible-builds.org/.

Use the following commands for each build system:

### Waf

```shell
./waf configure --enable-reproducible-builds
```

### Meson

```shell
meson setup builddir . -Denable_reproducible_builds=true
```

### CMake

```shell
cmake -G Ninja -B builddir -DCSP_REPRODUCIBLE_BUILDS=ON
```

Note: By default, CMake embeds the build directory in the binaries,
resulting in non-deterministic builds. To address this, use
`CMAKE_BUILD_RPATH_USE_ORIGIN=ON` or `CMAKE_SKIP_RPATH=ON` as follows:

```shell
cmake -G Ninja -B builddir -DCSP_REPRODUCIBLE_BUILDS=ON -DCMAKE_BUILD_RPATH_USE_ORIGIN=ON
```

See [Reproducible Builds site][1] or [CMake document][2] for more details.

[1]: https://reproducible-builds.org/docs/deterministic-build-systems/
[2]: https://cmake.org/cmake/help/latest/prop_tgt/BUILD_RPATH.html
