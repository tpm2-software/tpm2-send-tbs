UTIL_CC = gcc
CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -Wextra -Werror -D_WIN32_WINNT=0x0600
LDFLAGS = -L /mnt/c/Program\ Files\ \(x86\)/Windows\ Kits/10/Lib/*/um/x64
LDLIBS = -l:tbs.lib
MKDIR = mkdir -p
ZIP = zip
SRC_DIR = src
BUILD_DIR = build

define remove_trailing_slash
	$(patsubst %/,%,$(1))
endef

define msvc
    $(eval $@_CD = cd $(call remove_trailing_slash,$(dir $(4))))
    $(eval $@_SETUP = call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" $(1))
    $(eval $@_COMPILE = cl $(2) "$(shell wslpath -a -w  $(3))" /link tbs.lib /out:"$(notdir $(4))")
	cmd.exe /c ${$@_CD} "&&"${$@_SETUP}  "&&" ${$@_COMPILE}
endef

all: tpm2-send-tbs hex unhex

tpm2-send-tbs: $(BUILD_DIR)/tpm2-send-tbs.exe
hex: $(BUILD_DIR)/hex
unhex: $(BUILD_DIR)/unhex

$(BUILD_DIR)/tpm2-send-tbs.exe: $(SRC_DIR)/tpm2-send-tbs.c
	$(MKDIR) $(dir $@)
ifneq ($(shell which $(CC)),)
	@echo "== Detected MinGW, builing with $(CC)... =="
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)
else
	@echo "== No MinGW, attempting build with MSVC... =="
	$(call msvc,x64,/W4 /WX,$<,$@)
endif

$(BUILD_DIR)/hex: $(SRC_DIR)/hex.c
	$(MKDIR) $(dir $@)
	$(UTIL_CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/unhex: $(SRC_DIR)/unhex.c
	$(MKDIR) $(dir $@)
	$(UTIL_CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/tpm2-send-tbs_%_win_x64.zip: $(BUILD_DIR)/tpm2-send-tbs.exe
	$(ZIP) $@ $^

check: $(BUILD_DIR)/tpm2-send-tbs.exe $(BUILD_DIR)/hex $(BUILD_DIR)/unhex
	bash test/test.sh

release: $(BUILD_DIR)/tpm2-send_$(shell git tag --contains HEAD)_win_x64.zip

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all tpm2-send-tbs hex unhex check release clean
