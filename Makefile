## AAUSAT3 libcsp makefile

## Configuration
ARCH=posix
#ARCH=freertos
#TOOLCHAIN=bfin-linux-uclibc-
#TOOLCHAIN=avr-
TOOLCHAIN=
#TOOLCHAIN=arm-elf-
TARGET = libcsp.a
OUTDIR = .
MCU = at90can128
CC = $(TOOLCHAIN)gcc
AR = $(TOOLCHAIN)ar
SZ = $(TOOLCHAIN)size

## Options common to compile, link and assembly rules
COMMON =# -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON) -D_GNU_SOURCE
CFLAGS += -Wall -Werror -gdwarf-2 -std=gnu99 -O2 -funsigned-char -funsigned-bitfields# -fpack-struct -fshort-enums

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
#LDFLAGS = $(COMMON)
#LDFLAGS +=  -Wl,-Map=csptest.map

## Archiver flags
ARFLAGS = rcu

## Include Directories
INCLUDES = -I./include
INCLUDES += -I../aausat3/software/include/

## Objects that must be built in order to archive
SOURCES += src/csp_buffer.c
SOURCES += src/csp_conn.c
SOURCES += src/csp_if_lo.c
SOURCES += src/csp_io.c
SOURCES += src/csp_route.c
SOURCES += src/csp_port.c
SOURCES += src/csp_services.c
SOURCES += src/csp_endian.c
SOURCES += src/csp_debug.c
SOURCES += src/arch/$(ARCH)/csp_malloc.c
SOURCES += src/arch/$(ARCH)/csp_queue.c
SOURCES += src/arch/$(ARCH)/csp_semaphore.c
SOURCES += src/arch/$(ARCH)/csp_time.c
SOURCES += src/arch/$(ARCH)/csp_thread.c
SOURCES += src/arch/$(ARCH)/pthread_queue.c

OBJECTS=$(SOURCES:.c=.o)

# Define PHONY targets
.PHONY: all clean size

## Default target
all: $(SOURCES) $(TARGET) size

## Compile
.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

## Archive to create static lib
$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $(OUTDIR)/$(TARGET) $(OBJECTS) 

size:
	$(SZ) -t $(OUTDIR)/$(TARGET)

## Clean target
clean:
	-rm -rf $(OBJECTS) $(OUTDIR)/$(TARGET)
