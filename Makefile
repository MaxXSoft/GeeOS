# toolchain
include toolchain.mk

# helper functions
rwildcard = $$(foreach d, $$(wildcard $$(1:=/*)), $$(call rwildcard, $$d, $$2) $$(filter $$(subst *, %, $$2), $$d))
define make_obj
	$(eval TEMP_VAR := $(patsubst $(SRC_DIR)/%.yu, $(OBJ_DIR)/%.yu.o, $(2)));
	$(eval TEMP_VAR := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.c.o, $(TEMP_VAR)));
	$(eval TEMP_VAR := $(patsubst $(SRC_DIR)/%.S, $(OBJ_DIR)/%.S.o, $(TEMP_VAR)));
	$(eval $(1)_OBJ := $(TEMP_VAR));
endef

# directories
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
SRC_DIR := src
LIB_DIR := $(SRC_DIR)/lib
ARCH_DIR := $(SRC_DIR)/arch
BOOT_DIR := $(SRC_DIR)/boot

# source & targets of library
LIB_SRC := $(call rwildcard, $(LIB_DIR), *.yu)
LIB_SRC += $(call rwildcard, $(ARCH_DIR)/riscv, *.yu)
LIB_SRC += $(call rwildcard, $(ARCH_DIR)/riscv, *.S)
$(call make_obj, LIB, $(LIB_SRC))
LIB_TARGET := $(BUILD_DIR)/libgee.a

# source & targets of bootloader
BOOT_SRC := $(call rwildcard, $(BOOT_DIR), *.yu)
BOOT_SRC += $(call rwildcard, $(BOOT_DIR), *.S)
$(call make_obj, BOOT, $(BOOT_SRC))
BOOT_LDS := $(BOOT_DIR)/linker.ld
BOOT_TARGET := $(BUILD_DIR)/boot.bin

# source & targets of kernel
KERNEL_SRC := $(call rwildcard, $(SRC_DIR), *.yu)
KERNEL_SRC += $(call rwildcard, $(SRC_DIR), *.c)
KERNEL_SRC += $(call rwildcard, $(SRC_DIR), *.S)
KERNEL_SRC := $(filter-out $(LIB_SRC) $(BOOT_SRC), $(KERNEL_SRC))
KERNEL_SRC := $(filter-out $(call rwildcard, $(ARCH_DIR), *.*), $(KERNEL_SRC))
$(call make_obj, KERNEL, $(KERNEL_SRC))
KERNEL_LDS := $(SRC_DIR)/linker.ld
KERNEL_TARGET := $(BUILD_DIR)/geeos.elf


.PHONY: all clean libgee boot kernel

all: libgee boot kernel

clean:
	@echo "cleaning..."
	@-rm -rf $(OBJ_DIR)
	@-rm $(LIB_TARGET)
	@-rm $(BOOT_TARGET)
	@-rm $(BOOT_TARGET).*
	@-rm $(KERNEL_TARGET)
	@-rm $(KERNEL_TARGET).*

libgee: $(BUILD_DIR) $(LIB_TARGET)

boot: $(BUILD_DIR) $(BOOT_TARGET)

kernel: $(BUILD_DIR) $(KERNEL_TARGET)

$(BUILD_DIR):
	@mkdir $(BUILD_DIR)

$(LIB_TARGET): $(LIB_OBJ)
	@echo "making GeeOS kernel library..."
	@$(AR) $@ $^
	@$(RANLIB) $@

$(BOOT_TARGET): $(BOOT_TARGET).elf
	@echo "making GeeOS bootloader..."
	@$(OBJC) -j .text -j .data $^ $@
	@$(OBJD) $^ > $@.dump

$(BOOT_TARGET).elf: $(BOOT_OBJ) $(BOOT_LDS) $(LIB_TARGET)
	@$(LD) -T$(BOOT_LDS) -L$(BUILD_DIR) -lgee -o $@ $(BOOT_OBJ)

$(KERNEL_TARGET): $(KERNEL_OBJ) $(KERNEL_LDS) $(LIB_TARGET)
	@echo "making GeeOS kernel..."
	@$(LD) -T$(KERNEL_LDS) -L$(BUILD_DIR) -lgee -o $@ $(KERNEL_OBJ)
	@$(OBJD) $@ > $@.dump

$(OBJ_DIR)/%.yu.o: $(SRC_DIR)/%.yu
	@echo "YUC $@"
	@-mkdir -p $(dir $@)
	@$(YUC) -I $(SRC_DIR) -ot llvm $^ | $(LLC) -o $@

$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c
	@echo "CC  $@"
	@-mkdir -p $(dir $@)
	@$(CC) -I$(SRC_DIR) -o $@ $^

$(OBJ_DIR)/%.S.o: $(SRC_DIR)/%.S
	@echo "AS  $@"
	@-mkdir -p $(dir $@)
	@$(CC) -I$(SRC_DIR) -o $@ $^
