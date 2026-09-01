# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose

# Usage:   
PSROOT = ../..

include $(PSROOT)/build_scripts/platform.mk

ifeq ($(gtest),y)
TARGET = libplatform_gtest_$(BUILDCOMMON).a
else
TARGET = libplatform_$(BUILDCOMMON).a
endif


include ../../build_scripts/platform.mk

OBJECT += pf_mbox.o
OBJECT += pf_ex.o 
OBJECT += pf_log.o
OBJECT += pf_inter_msg_server.o
OBJECT += pf_timer_manager.o 
OBJECT += pf_timer_service_linux_signal.o
OBJECT += pf_stat.o
OBJECT += pf_thread_mon.o
OBJECT += pf_upgrade.o
OBJECT += pf_daily_record.o
OBJECT += pf_comm.o
OBJECT += pf_ftp.o
OBJECT += pf_rdkafka_consumer.o
OBJECT += pf_rdkafka_producer.o
OBJECT += pf_nacos_wrapper.o
OBJECT += pf_http_server.o
OBJECT += pf_http_client.o

.PHONY: all clean 


all:
	@echo "TARGET: " $(TARGET)
	rm -rf $(BUILD_DIR)/pf_log.*
include ../../build_scripts/staticlibrary.mk

clean:
	rm -rf $(BUILD_DIR)/pf_log.*
