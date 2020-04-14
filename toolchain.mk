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
export YUC := $(YU_BIN)/yuc $(YUFLAGS)

# C compiler
CFLAGS := -Wall -Werror -c -static $(C_DEBUG_ARG) $(C_OPT_ARG)
CFLAGS += -fno-builtin -fno-pic
CFLAGS += -target riscv32-unknown-elf -march=rv32ima -mabi=ilp32
export CC := $(LLVM_BIN)/clang $(CFLAGS)

# LLVM compiler
LLCFLAGS := $(C_OPT_ARG) -filetype=obj
LLCFLAGS += -march=riscv32 -mcpu=generic-rv32 -mattr=+m,+a
export LLC := $(LLVM_BIN)/llc $(LLCFLAGS)

# linker
LDFLAGS := -nostdlib -melf32lriscv
export LD := $(LLVM_BIN)/ld.lld $(LDFLAGS)

# objcopy
OBJCFLAGS := -O binary
export OBJC := $(LLVM_BIN)/llvm-objcopy $(OBJCFLAGS)

# objdump
OBJDFLAGS := -D
export OBJD := objdump $(OBJDFLAGS)

# archiver
ARFLAGS := ru
export AR := $(LLVM_BIN)/llvm-ar $(ARFLAGS)

# ranlib
RANLIBFLAGS :=
export RANLIB := $(LLVM_BIN)/llvm-ranlib $(RANLIBFLAGS)
