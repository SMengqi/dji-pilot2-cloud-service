# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose
#tdynamicload : main.c str_out.so
#	gcc main.c -o tdynamicload -rdynamic -ldl -fPIC

#str_out.so : str_out.c str_out.h
#	gcc -c str_out.c -o str_out.o -fPIC
#	gcc -Wall -W -shared -fPIC -o str_out.so str_out.o

PSROOT = ..
TARGETSO = libcmd_line.so
include ../build_scripts/platform.mk


OBJECT += command_line/command.o
OBJECT += command_line/cmdline_upgrade.o

include ../build_scripts/staticlibrary.mk
