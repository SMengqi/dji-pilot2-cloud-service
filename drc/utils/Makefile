# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose

PSROOT = ../..
TARGET = libcommon_utils_$(BUILDNAME).a
include $(PSROOT)/build_scripts/platform.mk

OBJECT += utm.o jsonUtil.o Geofencing.o
OBJECT += kafka/ProducerHandler.o kafka/ConsumerHandler.o
OBJECT += IniHanmingDistanceMap.o
OBJECT += FunTimer.o
OBJECT += DrcNacosWrapper.o
OBJECT += drc_http_client.o
include $(PSROOT)/build_scripts/staticlibrary.mk
