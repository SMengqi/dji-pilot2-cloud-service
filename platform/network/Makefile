# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose

# Usage:   make network
PSROOT = ../..

include $(PSROOT)/build_scripts/platform.mk

ifeq ($(gtest),y)
TARGET = libplnetwork_gtest_$(BUILDCOMMON).a
else
TARGET = libplnetwork_$(BUILDCOMMON).a
endif

INCFLAGS += -I$(PSROOT)/platform/network
INCFLAGS += -I$(PSROOT)/platform/network/Linux
INCFLAGS += -I$(PSROOT)/platform/network/Linux/sctp
INCFLAGS += -I$(PSROOT)/platform/network/Linux/pcap

#OBJECT +=  test_network.o
OBJECT +=  platform_netsession_client.o
OBJECT +=  platform_netsession_server.o
OBJECT +=  client_connection.o
OBJECT +=  cross_platform_socket.o
OBJECT +=  net_buffer.o
OBJECT +=  net_factory.o
OBJECT +=  net_lib.o
OBJECT +=  net_manager.o
OBJECT +=  netlib_error.o
OBJECT +=  platform_socket_factory.o
OBJECT +=  server_connection.o
#OBJECT +=  pcap_sender.o
OBJECT +=  net_arp.o
OBJECT +=  net_ipv4.o
#OBJECT +=  sctp_test.o
OBJECT +=  ipv4address.o
OBJECT +=  Linux/platform_io.o
OBJECT +=  Linux/platform_mutex.o
OBJECT +=  Linux/platform_signal.o
OBJECT +=  Linux/platform_socket.o
OBJECT +=  Linux/platform_thread.o
OBJECT +=  Linux/platform_net_tools.o
OBJECT +=  Linux/sctp/addrs.o
OBJECT +=  Linux/sctp/bindx.o
OBJECT +=  Linux/sctp/connectx.o
OBJECT +=  Linux/sctp/opt_info.o
OBJECT +=  Linux/sctp/peeloff.o
OBJECT +=  Linux/sctp/recvmsg.o
OBJECT +=  Linux/sctp/sendmsg.o



.PHONY: all clean 


all:
	@echo "TARGET: " $(TARGET)
	rm -rf $(BUILD_DIR)/platform_netsession_*.*

CFLAGS += -DPMAL_SG_SUPPORT -I$(KERNEL_PATH)/include -I$(KERNEL_PATH)/arch/powerpc/include
CXXFLAGS += -DPMAL_SG_SUPPORT -I$(KERNEL_PATH)/include -I$(KERNEL_PATH)/arch/powerpc/include

include $(PSROOT)/build_scripts/staticlibrary.mk


clean:
	rm -rf $(BUILD_DIR)/platform_netsession_*.*
