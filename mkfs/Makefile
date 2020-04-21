# sources & targets
MKFS_SRC := $(call rwildcard, $(MKFS_DIR), *.cpp)
$(call make_obj, MKFS, $(MKFS_SRC))
MKFS_TARGET := $(BUILD_DIR)/mkfs

# complier flags
CXXFLAGS := -I$(MKFS_DIR)


.PHONY: all clean mkfs

all: mkfs

clean:
	-rm $(MKFS_TARGET)

mkfs: $(MKFS_TARGET)

$(MKFS_TARGET): $(MKFS_OBJ)
	$(info making mkfs utility...)
	$(NLD) -o $@ $^

include $(TOP_DIR)/rules.mk
