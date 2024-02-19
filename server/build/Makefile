CPUS=$(shell cat /proc/cpuinfo | grep "processor" | wc -l)
PWD=$(shell pwd)
BUILD_DIR=$(PWD)/output
MAKE_OPT=

define shcmd-makepre
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..
endef

define shcmd-make
	@cd $(BUILD_DIR) && make -j$(CPUS) $(MAKE_OPT) | grep -v "^make\[[0-9]\]:"
endef

define shcmd-makeclean
	@cd $(BUILD_DIR) && make clean
endef

define shcmd-makerm
	rm -rf output
endef

.PHONY: all clean rm pre
all:
	$(call shcmd-make)
#	cp output/udb ../scripts/linux/udb
#	strip -s ../scripts/linux/udb
#	upx -9 ../scripts/linux/udb

clean:
	$(call shcmd-makeclean)

rm:
	$(call shcmd-makerm)

pre:
	$(call shcmd-makepre)
