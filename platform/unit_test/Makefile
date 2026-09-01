# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose

# Usage:   make INSTALL_DIR=/path/to/ecos/install

PSROOT = ../..
TARGET = libut_$(BUILDNAME).a
include ../../build_scripts/platform.mk

OBJECT += ut_main/ut_main.o
OBJECT += ut_main/ut_pthread.o
OBJECT += ut_main/ut_msg.o
OBJECT += ut_main/ut_cycles.o
OBJECT += ut_main/ut_unsocket.o
OBJECT += ut_main/ut_log.o
#OBJECT += ut_main/ut_ipsec.o
#OBJECT += ut_main/ut_cipherintegrity.o
OBJECT += soft_security/lte_cipher.o
OBJECT += soft_security/aes128/aes128_api_pdcp_dl.o
OBJECT += soft_security/aes128/aes128_api_pdcp_ul.o
OBJECT += soft_security/aes128/aes128_pdcp_dl.o
OBJECT += soft_security/aes128/aes128_pdcp_ul.o
OBJECT += soft_security/snow3g/snow3g_api_pdcp_dl.o
OBJECT += soft_security/snow3g/snow3g_api_pdcp_ul.o
OBJECT += soft_security/snow3g/snow3g_pdcp_dl.o
OBJECT += soft_security/snow3g/snow3g_pdcp_ul.o
OBJECT += soft_security/zuc/zuc_pdcp_ul.o
OBJECT += soft_security/zuc/zuc_pdcp_dl.o
#OBJECT += ut_main/ut_ipsec_pc.o

include ../../build_scripts/staticlibrary.mk
