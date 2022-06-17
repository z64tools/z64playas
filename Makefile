CFLAGS          = -Wall -Wno-switch -DEXTLIB=149 -DNDEBUG -I wren/src/include/ -I wren/src/optional/ -I wren/src/vm/
CFLAGS_MAIN     = -Wall -Wno-switch -DNDEBUG
OPT_WIN32      := -Ofast
OPT_LINUX      := -Ofast
SOURCE_C        = $(shell find src/* -type f -name '*.c')
SOURCE_C       += $(shell find wren/src/* -type f -name '*.c')
SOURCE_O_LINUX := $(foreach f,$(SOURCE_C:.c=.o),bin/linux/$f)
SOURCE_O_WIN32 := $(foreach f,$(SOURCE_C:.c=.o),bin/win32/$f)

RELEASE_EXECUTABLE_LINUX := z64playas
RELEASE_EXECUTABLE_WIN32 := z64playas.exe

PRNT_DGRY := \e[90;2m
PRNT_GRAY := \e[0;90m
PRNT_REDD := \e[0;91m
PRNT_GREN := \e[0;92m
PRNT_YELW := \e[0;93m
PRNT_BLUE := \e[0;94m
PRNT_PRPL := \e[0;95m
PRNT_CYAN := \e[0;96m
PRNT_RSET := \e[m

# Make build directories
$(shell mkdir -p bin/ $(foreach dir, \
	$(dir $(SOURCE_O_WIN32)) \
	$(dir $(SOURCE_O_LINUX)) \
	, $(dir)))

.PHONY: default \
		linux \
		win32 \
		clean

default: linux
all: linux win32
linux: $(RELEASE_EXECUTABLE_LINUX)
win32: $(RELEASE_EXECUTABLE_WIN32)

clean:
	rm -f -R bin

include $(C_INCLUDE_PATH)/ExtLib.mk

# # # # # # # # # # # # # # # # # # # #
# LINUX BUILD                         #
# # # # # # # # # # # # # # # # # # # #
	
bin/linux/%.o: %.c $(ExtLib_H) src/z64playas.h
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@gcc -c -o $@ $< $(OPT_LINUX) $(CFLAGS)

$(RELEASE_EXECUTABLE_LINUX): $(SOURCE_O_LINUX) $(ExtLib_Linux_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@gcc -o $@ $^ -lm -ldl -pthread $(OPT_LINUX) $(CFLAGS_MAIN)

# # # # # # # # # # # # # # # # # # # #
# WIN32 BUILD                         #
# # # # # # # # # # # # # # # # # # # #
	
bin/win32/%.o: %.c $(ExtLib_H) src/z64playas.h
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -c -o $@ $< $(OPT_WIN32) $(CFLAGS) -D_WIN32

bin/win32/icon.o: src/icon.rc src/icon.ico
	@i686-w64-mingw32.static-windres -o $@ $<

$(RELEASE_EXECUTABLE_WIN32): $(SOURCE_O_WIN32) $(ExtLib_Win32_O)
	@echo "$(PRNT_RSET)[$(PRNT_PRPL)$(notdir $@)$(PRNT_RSET)] [$(PRNT_PRPL)$(notdir $^)$(PRNT_RSET)]"
	@i686-w64-mingw32.static-gcc -o $@ $^ -lm -pthread $(OPT_WIN32) $(CFLAGS_MAIN) -D_WIN32
