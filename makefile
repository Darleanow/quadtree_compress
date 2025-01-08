# -----------------------------------------------------------------------------
# Project Configuration
# -----------------------------------------------------------------------------
NAME        := codec
VERSION     := 1.0.0
PREFIX      ?= /usr/local
DESTDIR     ?=

# -----------------------------------------------------------------------------
# Directory Structure
# -----------------------------------------------------------------------------
SRC_DIR     := src
INC_DIR     := include
BUILD_DIR   := build

SRCS        := $(shell find $(SRC_DIR) -name '*.c')
OBJS        := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
SRC_SUBDIRS := $(shell find $(SRC_DIR) -type d)
BUILD_SUBDIRS := $(SRC_SUBDIRS:$(SRC_DIR)%=$(BUILD_DIR)%)

DEPS        := $(OBJS:.o=.d)

# -----------------------------------------------------------------------------
# Tools and Commands
# -----------------------------------------------------------------------------
CC          := gcc
MKDIR       := mkdir -p
RM          := rm -f
RMDIR       := rm -rf

# -----------------------------------------------------------------------------
# Warning Flags
# -----------------------------------------------------------------------------
WARNINGS    := -Wall -Wextra -Werror \
               -Walloc-zero -Walloca \
               -Warray-bounds=2 -Wcast-align=strict \
               -Wcast-qual -Wconversion \
               -Wdangling-else -Wdate-time \
               -Wdisabled-optimization -Wdouble-promotion \
               -Wduplicated-branches -Wduplicated-cond \
               -Wfloat-equal -Wformat=2 -Wformat-nonliteral \
               -Wformat-security -Wformat-signedness \
               -Wframe-larger-than=4096 -Wimplicit-fallthrough \
               -Winit-self -Winline -Winvalid-pch \
               -Wlogical-op -Wmissing-declarations \
               -Wmissing-include-dirs -Wmissing-prototypes \
               -Wnested-externs -Wnull-dereference \
               -Wold-style-definition -Wpacked \
               -Wpointer-arith -Wredundant-decls \
               -Wrestrict -Wshadow -Wshift-overflow=2 \
               -Wsign-conversion -Wstack-protector \
               -Wstrict-prototypes -Wstringop-overflow=4 \
               -Wswitch-default -Wswitch-enum \
               -Wtrampolines -Wundef -Wuninitialized \
               -Wunused-macros -Wvla -Wwrite-strings

# -----------------------------------------------------------------------------
# Optimization and Security Flags
# -----------------------------------------------------------------------------
OPTIMIZE    := -O2 -fno-strict-aliasing \
               -fstack-protector-strong \
               -fcf-protection=full -fPIE -pthread \
               -fexceptions -ftrapv \
               -fno-delete-null-pointer-checks \
               -fno-strict-overflow \
               -fno-merge-all-constants -fno-plt \
               -fno-common \
               -ftrivial-auto-var-init=zero

# -----------------------------------------------------------------------------
# Features and Standards
# -----------------------------------------------------------------------------
FEATURES    := -std=c11 -pedantic-errors \
               -D_DEFAULT_SOURCE \
               -D_FILE_OFFSET_BITS=64 \
               -D_POSIX_C_SOURCE=200809L \
               -D_GNU_SOURCE \
               -DNDEBUG

# -----------------------------------------------------------------------------
# Linker Security Flags
# -----------------------------------------------------------------------------
LDFLAGS     := -Wl,-z,relro -Wl,-z,now \
               -Wl,-z,noexecstack \
               -Wl,-z,separate-code \
               -Wl,-z,defs \
               -pie -Wl,-pie

# -----------------------------------------------------------------------------
# Final Flags Assembly
# -----------------------------------------------------------------------------
INCLUDES    := -I$(INC_DIR)
DEPFLAGS    := -MMD -MP -MF $(BUILD_DIR)/$*.d

CFLAGS      := $(WARNINGS) $(OPTIMIZE) $(FEATURES) $(INCLUDES)

LIBS        := -lm

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------
.PHONY: all clean directories check-compiler

all: check-compiler directories $(NAME)

check-compiler:
	@echo "Checking GCC version..."
	@$(CC) --version
	@if [ ! -x "$$(command -v $(CC))" ]; then \
		echo "Error: GCC not found"; \
		exit 1; \
	fi

directories: $(BUILD_SUBDIRS)

$(BUILD_SUBDIRS):
	@$(MKDIR) $@

$(NAME): $(OBJS)
	@echo "Linking $(NAME)..."
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@
	@echo "Build complete!"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | directories
	@echo "Compiling $<..."
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning build files..."
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(NAME)

-include $(DEPS)