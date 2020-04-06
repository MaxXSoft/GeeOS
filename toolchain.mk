# external parameters
DEBUG = 1
OPT_LEVEL = 2

# judge if is debug mode
ifeq ($(DEBUG), 0)
	C_DEBUG_ARG = -DNDEBUG
	C_OPT_ARG = -O$(OPT_LEVEL)
	YU_OPT_ARG = -O $(OPT_LEVEL)
else
	C_DEBUG_ARG = -g
	C_OPT_ARG = -O0
	YU_OPT_ARG = -O 0
endif

# cross compile toolchain prefix
LLVM_BIN := /usr/local/opt/llvm/bin
YU_BIN := /Users/maxxing/Programming/MyRepo/YuLang/build

# Yu compiler
YUFLAGS := -Werror $(YU_OPT_ARG)
YUFLAGS += -tt riscv32-unknown-elf -tc generic-rv32 -tf +m,+a
YUC := $(YU_BIN)/yuc $(YUFLAGS)

# C compiler
CFLAGS := -Wall -Werror -c -static $(C_DEBUG_ARG) $(C_OPT_ARG)
# TODO: there is no '-mstrict-align' flag in clang
CFLAGS += -fno-builtin -fno-pic
CFLAGS += -target riscv32-unknown-elf -march=rv32ima -mabi=ilp32
CC := $(LLVM_BIN)/clang $(CFLAGS)

# linker
# LDFLAGS := -nostartfiles -nostdlib -nostdinc -melf32lriscv
LDFLAGS :=
LD := $(LLVM_BIN)/ld.lld $(LDFLAGS)

# objcopy
OBJCFLAGS := -O binary
OBJC := $(LLVM_BIN)/llvm-objcopy $(OBJCFLAGS)

# objdump
OBJDFLAGS := -D
OBJD := $(LLVM_BIN)/llvm-objdump $(OBJDFLAGS)

# archiver
ARFLAGS := ru
AR := $(LLVM_BIN)/llvm-ar $(ARFLAGS)

# ranlib
RANLIBFLAGS :=
RANLIB := $(LLVM_BIN)/llvm-ranlib $(RANLIBFLAGS)
