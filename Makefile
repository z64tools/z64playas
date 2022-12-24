ifeq (,$(wildcard settings.mk))
  $(error Please run ./setup.sh to automatically install ExtLib)
endif
include settings.mk

CFLAGS          = -Ofast -Wall -Wno-switch -DEXTLIB=220 -DNDEBUG -I wren/src/include/ -I wren/src/optional/ -I wren/src/vm/ -I dlcopy/src
SOURCE_C        = $(shell find src/* -type f -name '*.c')
SOURCE_C       += $(shell find wren/src/* -type f -name '*.c')
SOURCE_C       += $(shell find dlcopy/src/* -type f -name '*.c')
SOURCE_C       += src/main_module.c
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)

RELEASE_EXECUTABLE_LINUX := app_linux/z64playas
RELEASE_EXECUTABLE_WIN32 := app_win32/z64playas.exe

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(SOURCE_O_WIN32)) \
	$(dir $(SOURCE_O_LINUX)) \
	$(dir $(RELEASE_EXECUTABLE_LINUX)) \
	$(dir $(RELEASE_EXECUTABLE_WIN32)) \
	, $(dir)))

.PHONY: default \
		linux \
		win32 \
		clean

default: linux
all: linux win32
linux: $(RELEASE_EXECUTABLE_LINUX)
	@cp manifest/* app_linux/
win32: $(RELEASE_EXECUTABLE_WIN32)
	@cp manifest/* app_win32/

clean:
	rm -rf bin
	rm -f $(RELEASE_EXECUTABLE_LINUX)
	rm -f $(RELEASE_EXECUTABLE_WIN32)

include $(PATH_EXTLIB)/ext_lib.mk

src/main_module.c: src/main_module.mnf
	xxd -i $< $@
	sed -i -e 's/src_main_module_mnf/gMainModuleData/g' $@
	sed -i -e 's/gMainModuleData_len/gMainModuleSize/g' $@
	

bin/linux/wren/src/vm/wren_compiler.o: CFLAGS += -Wno-uninitialized
bin/linux/wren/src/vm/wren_vm.o: CFLAGS += -Wno-unused-variable
	
bin/win32/wren/src/vm/wren_compiler.o: CFLAGS += -Wno-uninitialized
bin/win32/wren/src/vm/wren_vm.o: CFLAGS += -Wno-unused-variable
bin/win32/wren/src/vm/wren_value.o: CFLAGS += -Wno-maybe-uninitialized

# # # # # # # # # # # # # # # # # # # #
# LINUX BUILD                         #
# # # # # # # # # # # # # # # # # # # #

-include $(SOURCE_O_LINUX:.o=.d)

bin/linux/%.o: %.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(CFLAGS)
	$(GD_LINUX)

$(RELEASE_EXECUTABLE_LINUX): $(SOURCE_O_LINUX) $(ExtLib_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ $(XFLAGS) $(CFLAGS)

# # # # # # # # # # # # # # # # # # # #
# WIN32 BUILD                         #
# # # # # # # # # # # # # # # # # # # #

-include $(SOURCE_O_WIN32:.o=.d)

bin/win32/%.o: %.c
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(CFLAGS) -D_WIN32
	$(GD_WIN32)

$(RELEASE_EXECUTABLE_WIN32): bin/win32/icon.o $(SOURCE_O_WIN32) $(ExtLib_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ $(XFLAGS) $(CFLAGS) -D_WIN32
