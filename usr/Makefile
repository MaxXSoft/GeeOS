# directories
LIB_DIR := $(USR_DIR)/lib
BIN_DIR := $(USR_DIR)/bin
BIN_OBJ_DIR := $(OBJ_DIR)/usr/bin
BIN_TARGET_DIR := $(BUILD_DIR)/usr

# sources & targets of library
LIB_SRC := $(call rwildcard, $(LIB_DIR), *.yu)
LIB_SRC += $(call rwildcard, $(LIB_DIR), *.S)
$(call make_obj, LIB, $(LIB_SRC))
LIB_TARGET := $(BUILD_DIR)/libgrt.a

# sources & targets of user binaries
BIN_SRC := $(call rwildcard, $(BIN_DIR), *.yu)
$(call make_obj, BIN, $(BIN_SRC))
BIN_TARGET := $(patsubst $(BIN_OBJ_DIR)/%.yu.o, $(BIN_TARGET_DIR)/%, $(BIN_OBJ))

# compiler flags
YUCFLAGS := -I $(USR_DIR)
CFLAGS := -I$(USR_DIR)


.PHONY: all clean libgrt user

all: libgrt user

clean:
	-rm $(LIB_TARGET)
	-rm -rf $(BIN_TARGET_DIR)

libgrt: $(LIB_TARGET)

user: $(BIN_TARGET_DIR) $(BIN_TARGET)

$(BIN_TARGET_DIR):
	mkdir $@

$(LIB_TARGET): $(LIB_OBJ)
	$(info making GeeOS user library...)
	$(AR) $@ $^
	$(RANLIB) $@

$(BIN_TARGET_DIR)/%: $(BIN_OBJ_DIR)/%.yu.o
	$(info making user binary $(basename $@)...)
	$(LD) -L$(BUILD_DIR) -lgrt -o $@ $^
	$(OBJD) $@ > $@.dump

include $(TOP_DIR)/rules.mk
