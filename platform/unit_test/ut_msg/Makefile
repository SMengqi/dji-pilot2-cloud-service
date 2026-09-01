# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose

# Usage:   make INSTALL_DIR=/path/to/ecos/install

PSROOT = ../../..
TARGET = libut_$(BUILDNAME).a
include ../../../build_scripts/platform.mk

OBJECT += ut_main.o
OBJECT += ut_msg.o
OBJECT += rd_producer.o
OBJECT += rd_consumer.o
OBJECT += flyto_controller_test.o


include ../../../build_scripts/staticlibrary.mk
