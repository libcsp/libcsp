## AAUSAT3 libcsp makefile

## Configuration
ifndef ARCH
ARCH = posix
endif

ifndef TOOLCHAIN
TOOLCHAIN = 
endif

TARGET = libcsp.a
OUTDIR = .
MCU = at90can128
CC = $(TOOLCHAIN)gcc
AR = $(TOOLCHAIN)ar
SZ = $(TOOLCHAIN)size

## Options common to compile, link and assembly rules

ifeq ($(TOOLCHAIN),avr-)
COMMON +=-mmcu=$(MCU)
else 
COMMON =
endif

## Compile options common for all C compilation units.
CFLAGS =$(COMMON)
ifeq ($(TOOLCHAIN),avr-)
CFLAGS +=-fpack-struct -fshort-enums
else ifeq ($(TOOLCHAIN),bfin-linux-uclibc-)
CFLAGS += -D_GNU_SOURCE
endif
CFLAGS += -Wall -Werror -gdwarf-2 -std=gnu99 -O2 -funsigned-char -funsigned-bitfields

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Archiver flags
ARFLAGS = -rcu

## Include Directories
INCLUDES = -I./include
INCLUDES += -I../aausat3/software/lib/libfreertos/include/

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

## POSIX archs requires pthread_queue
ifeq ($(ARCH),posix)
SOURCES += src/arch/$(ARCH)/pthread_queue.c
endif

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
