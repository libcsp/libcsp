# Simple Send via USART

This is a simple sample code to send and receive a file using SFP over LOOPBACK interface.

## How to Build

```
$ cmake -B builddir
$ ninja -C builddir simple-sfp-send-recv
```

## How to Test

In a terminal, run `simple-sfp-send-recv`

```
./builddir/samples/posix/simple-sfp-send-recv/simple-sfp-send-recv existing_file_on_system.txt file_to_create_on_system.txt
```

If it runs successfully, the program exits with 0.
