# Originally written by Jonathan Larmour, Red Hat, Inc.
# Modify by lijian (@Sunnorth) for easy building.
# This file is in the public domain and may be used for any purpose

BUILD_SCRIPTS_DIR = .

xtest = no

.PHONY: all clean CLOUDtrans

all:
	@echo "what do you want to build cloud? "
	@echo "for 5G-A PROCESS, type: make cloud"
	


cloud:
	@cp $(BUILD_SCRIPTS_DIR)/platform$@.mk $(BUILD_SCRIPTS_DIR)/platform.mk
	make -f $(BUILD_SCRIPTS_DIR)/Makefile.all 2>&1 |tee $@_build_$(shell date +%Y_%m_%d;).log


allclean:
	@cp $(BUILD_SCRIPTS_DIR)/platformcloud.mk $(BUILD_SCRIPTS_DIR)/platform.mk
	make -f $(BUILD_SCRIPTS_DIR)/Makefile.all clean


clean:
	make -f $(BUILD_SCRIPTS_DIR)/Makefile.all clean
