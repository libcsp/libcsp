# Makefile for libCSP (Simplified - Linux POSIX only)
#
# This Makefile is a simplified conversion from the CMakeLists.txt.
# It does NOT replicate all features, options, and platform detection
# of the original CMake setup. It primarily focuses on building the
# core 'csp' static library on Linux (POSIX).

# --- Configuration Options (Adjust as needed) ---

include ../phoenix-rtos-build/Makefile.common

NAME = libcsp

# Build Type: Debug, Release, MinSizeRel
BUILD_TYPE ?= Debug

# CSP Configuration Macros (from CMake options)
# Set 1 for ON, 0 for OFF
CSP_REPRODUCIBLE_BUILDS ?= 0
CSP_HAVE_STDIO ?= 1
CSP_ENABLE_CSP_PRINT ?= 1
CSP_PRINT_STDIO ?= 1
CSP_USE_RDP ?= 1
CSP_USE_HMAC ?= 1
CSP_USE_PROMISC ?= 1
CSP_USE_RTABLE ?= 0
CSP_BUFFER_ZERO_CLEAR ?= 1
CSP_PHOENIX ?= 1 # Assumed for this Makefile

# CSP Size Parameters (from CMake cache variables)
CSP_QFIFO_LEN ?= 15
CSP_PORT_MAX_BIND ?= 16
CSP_CONN_RXQUEUE_LEN ?= 16
CSP_CONN_MAX ?= 8
CSP_BUFFER_SIZE ?= 256
CSP_BUFFER_COUNT ?= 15
CSP_RDP_MAX_WINDOW ?= 5
CSP_RTABLE_SIZE ?= 10

CSP_BUFFER_SIZE = 100
CSP_BUFFER_COUNT = 10
CSP_CONN_MAX = 10



# Source and Build Directories
SRCDIR = src

# Source Files (from src/ directory)
# This list needs to be updated if new source files are added to the project.
LOCAL_SRCS = $(filter-out $(SRCDIR)/csp_yaml.c,$(wildcard $(SRCDIR)/*.c)) \
	$(SRCDIR)/arch/phoenixrtos/csp_semaphore.c \
	$(SRCDIR)/arch/phoenixrtos/csp_system.c \
	$(SRCDIR)/arch/posix/csp_clock.c \
	$(SRCDIR)/arch/posix/csp_queue.c \
	$(SRCDIR)/arch/posix/csp_time.c \
	$(SRCDIR)/arch/posix/pthread_queue.c \
	$(wildcard $(SRCDIR)/interfaces/*.c) \
	$(wildcard $(SRCDIR)/crypto/*.c) \
	$(SRCDIR)/drivers/can/can_grlibCan.c

# 	$(SRCDIR)/csp_rtable_cidr.c \

# Compiler Flags
# Removed: -Wpedantic
LOCAL_CFLAGS = -std=c11 -Wall -Wextra -Wshadow -Wcast-align \
	-Wpointer-arith -Wwrite-strings -Wno-unused-parameter \
	-DCSP_VERSION_MAJOR=2 -DCSP_VERSION_MINOR=1 \
	-Wno-undef \
	-Isrc/arch/posix -Isrc -Isrc/interfaces -Isrc/crypto -Iinclude/csp/drivers

# # Add CSP option macros
LOCAL_CFLAGS += \
	-DCSP_REPRODUCIBLE_BUILDS=$(CSP_REPRODUCIBLE_BUILDS) \
	-DCSP_HAVE_STDIO=$(CSP_HAVE_STDIO) \
	-DCSP_ENABLE_CSP_PRINT=$(CSP_ENABLE_CSP_PRINT) \
	-DCSP_PRINT_STDIO=$(CSP_PRINT_STDIO) \
	-DCSP_USE_RDP=$(CSP_USE_RDP) \
	-DCSP_USE_HMAC=$(CSP_USE_HMAC) \
	-DCSP_USE_PROMISC=$(CSP_USE_PROMISC) \
	-DCSP_USE_RTABLE=$(CSP_USE_RTABLE) \
	-DCSP_BUFFER_ZERO_CLEAR=$(CSP_BUFFER_ZERO_CLEAR) \
	-DCSP_PHOENIX=$(CSP_PHOENIX) \
	-DCSP_QFIFO_LEN=$(CSP_QFIFO_LEN) \
	-DCSP_PORT_MAX_BIND=$(CSP_PORT_MAX_BIND) \
	-DCSP_CONN_RXQUEUE_LEN=$(CSP_CONN_RXQUEUE_LEN) \
	-DCSP_BUFFER_SIZE=$(CSP_BUFFER_SIZE) \
	-DCSP_BUFFER_COUNT=$(CSP_BUFFER_COUNT)\
	-DCSP_CONN_MAX=$(CSP_CONN_MAX) \
	-DCSP_RDP_MAX_WINDOW=$(CSP_RDP_MAX_WINDOW) \
	-DCSP_RTABLE_SIZE=$(CSP_RTABLE_SIZE)

include $(static-lib.mk)

NAME := libcsp-test-loopback
LOCAL_SRCS = examples/csp_phoenix_test_loopback.c
DEP_LIBS := libcsp
LOCAL_CFLAGS = -Wno-undef 
LOCAL_CFLAGS += \
	-DCSP_REPRODUCIBLE_BUILDS=$(CSP_REPRODUCIBLE_BUILDS) \
	-DCSP_HAVE_STDIO=$(CSP_HAVE_STDIO) \
	-DCSP_ENABLE_CSP_PRINT=$(CSP_ENABLE_CSP_PRINT) \
	-DCSP_PRINT_STDIO=$(CSP_PRINT_STDIO) \
	-DCSP_USE_RDP=$(CSP_USE_RDP) \
	-DCSP_USE_HMAC=$(CSP_USE_HMAC) \
	-DCSP_USE_PROMISC=$(CSP_USE_PROMISC) \
	-DCSP_USE_RTABLE=$(CSP_USE_RTABLE) \
	-DCSP_BUFFER_ZERO_CLEAR=$(CSP_BUFFER_ZERO_CLEAR) \
	-DCSP_PHOENIX=$(CSP_PHOENIX) \
	-DCSP_QFIFO_LEN=$(CSP_QFIFO_LEN) \
	-DCSP_PORT_MAX_BIND=$(CSP_PORT_MAX_BIND) \
	-DCSP_CONN_RXQUEUE_LEN=$(CSP_CONN_RXQUEUE_LEN) \
	-DCSP_BUFFER_SIZE=$(CSP_BUFFER_SIZE) \
	-DCSP_BUFFER_COUNT=$(CSP_BUFFER_COUNT)\
	-DCSP_CONN_MAX=$(CSP_CONN_MAX) \
	-DCSP_RDP_MAX_WINDOW=$(CSP_RDP_MAX_WINDOW) \
	-DCSP_RTABLE_SIZE=$(CSP_RTABLE_SIZE)
LOCAL_HEADERS_DIR := nothing
include $(binary.mk)

# NAME := libcsp-test-server
# LOCAL_SRCS = examples/csp_phoenix_server_test.c
# DEP_LIBS := libcsp
# LIBS = grlib-can-core
# LOCAL_CFLAGS = -Wno-undef 
# LOCAL_CFLAGS += \
# 	-DCSP_REPRODUCIBLE_BUILDS=$(CSP_REPRODUCIBLE_BUILDS) \
# 	-DCSP_HAVE_STDIO=$(CSP_HAVE_STDIO) \
# 	-DCSP_ENABLE_CSP_PRINT=$(CSP_ENABLE_CSP_PRINT) \
# 	-DCSP_PRINT_STDIO=$(CSP_PRINT_STDIO) \
# 	-DCSP_USE_RDP=$(CSP_USE_RDP) \
# 	-DCSP_USE_HMAC=$(CSP_USE_HMAC) \
# 	-DCSP_USE_PROMISC=$(CSP_USE_PROMISC) \
# 	-DCSP_USE_RTABLE=$(CSP_USE_RTABLE) \
# 	-DCSP_BUFFER_ZERO_CLEAR=$(CSP_BUFFER_ZERO_CLEAR) \
# 	-DCSP_PHOENIX=$(CSP_PHOENIX) \
# 	-DCSP_QFIFO_LEN=$(CSP_QFIFO_LEN) \
# 	-DCSP_PORT_MAX_BIND=$(CSP_PORT_MAX_BIND) \
# 	-DCSP_CONN_RXQUEUE_LEN=$(CSP_CONN_RXQUEUE_LEN) \
# 	-DCSP_BUFFER_SIZE=$(CSP_BUFFER_SIZE) \
# 	-DCSP_BUFFER_COUNT=$(CSP_BUFFER_COUNT)\
# 	-DCSP_CONN_MAX=$(CSP_CONN_MAX) \
# 	-DCSP_RDP_MAX_WINDOW=$(CSP_RDP_MAX_WINDOW) \
# 	-DCSP_RTABLE_SIZE=$(CSP_RTABLE_SIZE)
# LOCAL_HEADERS_DIR := nothing
# include $(binary.mk)

# NAME := libcsp-test-client
# LOCAL_SRCS = examples/csp_phoenix_client_test.c
# DEP_LIBS := libcsp
# LIBS = grlib-can-core
# LOCAL_CFLAGS = -Wno-undef 
# LOCAL_CFLAGS += \
# 	-DCSP_REPRODUCIBLE_BUILDS=$(CSP_REPRODUCIBLE_BUILDS) \
# 	-DCSP_HAVE_STDIO=$(CSP_HAVE_STDIO) \
# 	-DCSP_ENABLE_CSP_PRINT=$(CSP_ENABLE_CSP_PRINT) \
# 	-DCSP_PRINT_STDIO=$(CSP_PRINT_STDIO) \
# 	-DCSP_USE_RDP=$(CSP_USE_RDP) \
# 	-DCSP_USE_HMAC=$(CSP_USE_HMAC) \
# 	-DCSP_USE_PROMISC=$(CSP_USE_PROMISC) \
# 	-DCSP_USE_RTABLE=$(CSP_USE_RTABLE) \
# 	-DCSP_BUFFER_ZERO_CLEAR=$(CSP_BUFFER_ZERO_CLEAR) \
# 	-DCSP_PHOENIX=$(CSP_PHOENIX) \
# 	-DCSP_QFIFO_LEN=$(CSP_QFIFO_LEN) \
# 	-DCSP_PORT_MAX_BIND=$(CSP_PORT_MAX_BIND) \
# 	-DCSP_CONN_RXQUEUE_LEN=$(CSP_CONN_RXQUEUE_LEN) \
# 	-DCSP_BUFFER_SIZE=$(CSP_BUFFER_SIZE) \
# 	-DCSP_BUFFER_COUNT=$(CSP_BUFFER_COUNT)\
# 	-DCSP_CONN_MAX=$(CSP_CONN_MAX) \
# 	-DCSP_RDP_MAX_WINDOW=$(CSP_RDP_MAX_WINDOW) \
# 	-DCSP_RTABLE_SIZE=$(CSP_RTABLE_SIZE)
# LOCAL_HEADERS_DIR := nothing
# include $(binary.mk)


all: libcsp libcsp-test-loopback
install: $(patsubst %,%-install,libcsp) $(patsubst %,%-install,libcsp-test-loopback)
clean: $(patsubst %,%-clean,libcsp) $(patsubst %,%-clean,libcsp-test-loopback)

# all: libcsp libcsp-test-loopback libcsp-test-server libcsp-test-client
# install: $(patsubst %,%-install,libcsp) $(patsubst %,%-install,libcsp-test-loopback) $(patsubst %,%-install,libcsp-test-server) $(patsubst %,%-install,libcsp-test-client)
# clean: $(patsubst %,%-clean,libcsp) $(patsubst %,%-clean,libcsp-test-loopback) $(patsubst %,%-clean,libcsp-test-server) $(patsubst %,%-install,libcsp-test-client)