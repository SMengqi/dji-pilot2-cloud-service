# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose

PSROOT = ../..
TARGET = libdevice_$(BUILDCOMMON).a
include $(PSROOT)/build_scripts/platform.mk

OBJECT += deviceMain.o
OBJECT += rcProperties.o

include $(PSROOT)/build_scripts/staticlibrary.mk
