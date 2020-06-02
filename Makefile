# toolchain
include toolchain.mk

# helper functions
export rwildcard = $$(foreach d, $$(wildcard $$(1:=/*)), $$(call rwildcard, $$d, $$2) $$(filter $$(subst *, %, $$2), $$d))
define make_obj
	$$(eval TEMP := $$(patsubst $(TOP_DIR)/%.yu, $(OBJ_DIR)/%.yu.o, $$(2)));
	$$(eval TEMP := $$(patsubst $(TOP_DIR)/%.c, $(OBJ_DIR)/%.c.o, $$(TEMP)));
	$$(eval TEMP := $$(patsubst $(TOP_DIR)/%.cpp, $(OBJ_DIR)/%.cpp.o, $$(TEMP)));
	$$(eval TEMP := $$(patsubst $(TOP_DIR)/%.S, $(OBJ_DIR)/%.S.o, $$(TEMP)));
	$$(eval $$(1)_OBJ := $$(TEMP));
endef
export make_obj

# directories
export TOP_DIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
export BUILD_DIR := $(TOP_DIR)/build
export SRC_DIR := $(TOP_DIR)/src
export USR_DIR := $(TOP_DIR)/usr
export MKFS_DIR := $(TOP_DIR)/mkfs
export UTILS_DIR := $(TOP_DIR)/utils
export OBJ_DIR := $(BUILD_DIR)/obj

# all sub-makes
SUB_MAKE := $(SRC_DIR) $(USR_DIR) $(MKFS_DIR)


.SILENT:
.PHONY: all clean libgee boot kernel libgrt user mkfs $(SUB_MAKE)

all: libgee boot kernel libgrt user mkfs

clean:
	$(info cleaning...)
	-rm -rf $(OBJ_DIR)
	-$(MAKE) -C $(SRC_DIR) $@
	-$(MAKE) -C $(USR_DIR) $@
	-$(MAKE) -C $(MKFS_DIR) $@

libgee: $(BUILD_DIR) $(SRC_DIR)

boot: $(BUILD_DIR) $(SRC_DIR)

kernel: $(BUILD_DIR) $(SRC_DIR)

libgrt: $(BUILD_DIR) $(USR_DIR)

user: $(BUILD_DIR) $(USR_DIR)

mkfs: $(BUILD_DIR) $(MKFS_DIR)

$(SUB_MAKE):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(SRC_DIR): $(USR_DIR)

$(USR_DIR): $(MKFS_DIR)

$(BUILD_DIR):
	mkdir $@
