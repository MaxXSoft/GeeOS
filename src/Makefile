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
BOOT_SRC := $(filter-out $(BOOT_DIR)/fuxi_sim.S, $(BOOT_SRC))
ifeq ($(TARGET),fuxi_sim)
BOOT_SRC := $(BOOT_DIR)/fuxi_sim.S
endif
$(call make_obj, BOOT, $(BOOT_SRC))
BOOT_LDS := $(BOOT_DIR)/linker.ld
BOOT_TARGET := $(BUILD_DIR)/boot.bin

# sources & targets of kernel
KERNEL_SRC := $(call rwildcard, $(SRC_DIR), *.yu)
KERNEL_SRC += $(call rwildcard, $(SRC_DIR), *.c)
KERNEL_SRC += $(call rwildcard, $(SRC_DIR), *.S)
KERNEL_SRC := $(filter-out $(LIB_SRC) $(call rwildcard, $(BOOT_DIR), *.*), $(KERNEL_SRC))
KERNEL_SRC := $(filter-out $(call rwildcard, $(ARCH_DIR), *.*), $(KERNEL_SRC))
$(call make_obj, KERNEL, $(KERNEL_SRC))
KERNEL_INIT := $(SRC_DIR)/init.S
$(call make_obj, KERNEL_INIT, $(KERNEL_INIT))
KERNEL_LDS := $(SRC_DIR)/linker.ld
USER_IMG := $(BUILD_DIR)/user.img
KERNEL_TARGET := $(BUILD_DIR)/geeos.elf

# compiler flags
YUCFLAGS := -I $(SRC_DIR) -D GEEOS_TARGET=$(TARGET)
CFLAGS := -I$(SRC_DIR)
ASFLAGS := -I$(BUILD_DIR)

# Make does not track compiler arguments. Update this file only when the
# selected platform changes, including when switching back to an earlier one.
TARGET_CONFIG := $(OBJ_DIR)/geeos-target
YU_OBJ := $(filter %.yu.o, $(LIB_OBJ) $(BOOT_OBJ) $(KERNEL_OBJ))


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

# Mark changed configurations phony so even Make versions with one-second
# timestamp resolution rebuild objects when targets are switched rapidly.
ifneq ($(shell cat $(TARGET_CONFIG) 2>/dev/null),$(TARGET))
.PHONY: $(TARGET_CONFIG)
endif

$(TARGET_CONFIG):
	mkdir -p $(dir $@)
	printf '%s\n' '$(TARGET)' > $@

$(YU_OBJ): $(TARGET_CONFIG)

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
	$(PYTHON) $(UTILS_DIR)/bin2coe.py $@

$(BOOT_TARGET).elf: $(BOOT_OBJ) $(BOOT_LDS) $(LIB_TARGET) $(TARGET_CONFIG)
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

-include $(addsuffix .d, $(YU_OBJ))
