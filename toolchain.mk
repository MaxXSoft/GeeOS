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
LLVM_BIN ?= $(shell command -v llvm-config | sed 's,/[^/]*$$,,')
YU_BIN ?= $(abspath ../YuLang/build)
LLD ?= $(LLVM_BIN)/ld.lld

# Keep RV32IMA available for simulator comparisons in separate build directories.
RISCV_ARCH ?= rv32imac
RISCV_FEATURES_rv32ima := +m,+a
RISCV_FEATURES_rv32imac := +m,+a,+c
ifeq ($(RISCV_FEATURES_$(RISCV_ARCH)),)
$(error unsupported RISCV_ARCH '$(RISCV_ARCH)'; expected rv32ima or rv32imac)
endif

# cross Yu compiler
YUFLAGS := -Werror $(YU_OPT_ARG)
YUFLAGS += -tt riscv32-unknown-elf -tc generic-rv32 -tf $(RISCV_FEATURES_$(RISCV_ARCH))
export YUC := $(YU_BIN)/yuc $(YUFLAGS)

# cross C compiler
CFLAGS := -Wall -Werror -c -static $(C_DEBUG_ARG) $(C_OPT_ARG)
CFLAGS += -fno-builtin -fno-pic
CFLAGS += -target riscv32-unknown-elf -march=$(RISCV_ARCH) -mabi=ilp32
export CC := $(LLVM_BIN)/clang $(CFLAGS)

# native C++ compiler
CXXFLAGS := -Wall -Werror -c $(C_DEBUG_ARG) $(C_OPT_ARG)
CXXFLAGS += -std=c++17
export CXX := clang++ $(CXXFLAGS)

# cross linker
LDFLAGS := -nostdlib -melf32lriscv
export LD := $(LLD) $(LDFLAGS)

# native linker
NLDFLAGS :=
export NLD := clang++ $(NLDFLAGS)

# objcopy
OBJCFLAGS := -O binary
export OBJC := $(LLVM_BIN)/llvm-objcopy $(OBJCFLAGS)

# objdump
OBJDFLAGS := -D
export OBJD := $(LLVM_BIN)/llvm-objdump $(OBJDFLAGS)

# strip
STRIPFLAGS := --strip-unneeded --strip-sections
export STRIP := $(LLVM_BIN)/llvm-strip $(STRIPFLAGS)

# archiver
ARFLAGS := ru
export AR := $(LLVM_BIN)/llvm-ar $(ARFLAGS)

# ranlib
RANLIBFLAGS :=
export RANLIB := $(LLVM_BIN)/llvm-ranlib $(RANLIBFLAGS)

# Python 3
export PYTHON ?= python3
