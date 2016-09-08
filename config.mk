# List of C source files.
CSRCS += $(wildcard $(LIBCSP_PATH)/src/*.c) \
         $(wildcard $(LIBCSP_PATH)/src/arch/freertos/*.c) \
         $(wildcard $(LIBCSP_PATH)/src/crypto/*.c) \
         $(wildcard $(LIBCSP_PATH)/src/transport/*.c) \
         $(LIBCSP_PATH)/src/interfaces/csp_if_can.c \
         $(LIBCSP_PATH)/src/interfaces/csp_if_lo.c \
         $(LIBCSP_PATH)/src/rtable/csp_rtable_cidr.c \

# List of include paths.
INC_PATH += $(LIBCSP_PATH)/include
