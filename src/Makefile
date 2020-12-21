# directories
LIB_DIR := $(SRC_DIR)/lib
ARCH_DIR := $(SRC_DIR)/arch
BOOT_DIR := $(SRC_DIR)/boot

# sources & targets of library
LIB_SRC := $(call rwildcard, $(LIB_DIR), *.yu)
LIB_SRC += $(call rwildcard, $(ARCH_DIR)/riscv, *.yu)
LIB_SRC += $(call rwildcard, $(ARCH_DIR)/riscv, *.S)
$(call make_obj, LIB, $(LIB_SRC))
LIB_TARGET := $(BUILD_DIR)/libgee.a

# sources & targets of bootloader
BOOT_SRC := $(call rwildcard, $(BOOT_DIR), *.yu)
BOOT_SRC += $(call rwildcard, $(BOOT_DIR), *.S)
$(call make_obj, BOOT, $(BOOT_SRC))
BOOT_LDS := $(BOOT_DIR)/linker.ld
BOOT_TARGET := $(BUILD_DIR)/boot.bin

# sources & targets of kernel
KERNEL_SRC := $(call rwildcard, $(SRC_DIR), *.yu)
KERNEL_SRC += $(call rwildcard, $(SRC_DIR), *.c)
KERNEL_SRC += $(call rwildcard, $(SRC_DIR), *.S)
KERNEL_SRC := $(filter-out $(LIB_SRC) $(BOOT_SRC), $(KERNEL_SRC))
KERNEL_SRC := $(filter-out $(call rwildcard, $(ARCH_DIR), *.*), $(KERNEL_SRC))
KERNEL_SRC := $(filter-out $(call rwildcard, $(SRC_DIR)/fs/ucore, *.*), $(KERNEL_SRC))
$(call make_obj, KERNEL, $(KERNEL_SRC))
KERNEL_INIT := $(SRC_DIR)/init.S
$(call make_obj, KERNEL_INIT, $(KERNEL_INIT))
KERNEL_LDS := $(SRC_DIR)/linker.ld
USER_IMG := $(BUILD_DIR)/user.img
KERNEL_TARGET := $(BUILD_DIR)/geeos.elf

# compiler flags
YUCFLAGS := -I $(SRC_DIR)
CFLAGS := -I$(SRC_DIR)
ASFLAGS := -I$(BUILD_DIR)


.PHONY: all clean libgee boot kernel

all: libgee boot kernel

clean:
	-rm $(LIB_TARGET)
	-rm $(BOOT_TARGET)
	-rm $(BOOT_TARGET).*
	-rm $(KERNEL_TARGET)
	-rm $(KERNEL_TARGET).*

libgee: $(LIB_TARGET)

boot: $(BOOT_TARGET)

kernel: $(KERNEL_TARGET)

$(LIB_TARGET): $(LIB_OBJ)
	$(info making GeeOS kernel library...)
	$(AR) $@ $^
	$(RANLIB) $@

$(BOOT_TARGET): $(BOOT_TARGET).elf
	$(info making GeeOS bootloader...)
	# create bin file
	$(OBJC) -j .text -j .data $^ $@
	# create dump
	$(OBJD) $^ > $@.dump
	# create coe file
	$(UTILS_DIR)/bin2coe.py $@

$(BOOT_TARGET).elf: $(BOOT_OBJ) $(BOOT_LDS) $(LIB_TARGET)
	$(LD) -T$(BOOT_LDS) -L$(BUILD_DIR) -lgee -o $@ $(BOOT_OBJ)

$(KERNEL_TARGET): $(KERNEL_OBJ) $(KERNEL_LDS) $(LIB_TARGET)
	$(info making GeeOS kernel...)
	$(LD) -T$(KERNEL_LDS) -L$(BUILD_DIR) -lgee -o $@ $(KERNEL_OBJ)
	$(OBJD) $@ > $@.dump
	$(if $(filter 0, $(DEBUG)), $(STRIP) $@ -o $@)

$(KERNEL_INIT_OBJ): $(KERNEL_INIT) $(USER_IMG)
	$(info AS  $@)
	-mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -o $@ $<

include $(TOP_DIR)/rules.mk
