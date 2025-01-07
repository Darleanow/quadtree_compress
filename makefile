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
CC          ?= gcc
MKDIR       := mkdir -p
RM          := rm -f
RMDIR       := rm -rf

# -----------------------------------------------------------------------------
# Compiler Flags
# -----------------------------------------------------------------------------
WARNINGS    := -Wall -Wextra \
               -Wconversion -Wimplicit-fallthrough \
               -Wmissing-prototypes -Wstrict-prototypes \
               -Wold-style-definition -Wmissing-declarations \
               -Wredundant-decls -Wnull-dereference \
               -Wdouble-promotion -Wfloat-equal -Wundef \
               -Wshadow -Wcast-align -Wwrite-strings \
               -Wbad-function-cast -Wpointer-arith \
               -Wuninitialized -Wno-unused-parameter

OPTIMIZE    := -O2 -fno-strict-aliasing -fno-common \
               -fstack-protector-strong -fstack-clash-protection \
               -fcf-protection=full -fPIE -pthread \
               -fno-plt -fexceptions

FEATURES    := -std=c11 -D_UNICODE -DUNICODE \
               -D_FORTIFY_SOURCE=2 -D_DEFAULT_SOURCE \
               -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809L

INCLUDES    := -I$(INC_DIR)
DEPFLAGS    := -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d

CFLAGS      := $(WARNINGS) $(OPTIMIZE) $(FEATURES) $(INCLUDES)

LIBS        := -lm

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------
.PHONY: all clean directories

all: directories $(NAME)

directories: $(BUILD_SUBDIRS)

$(BUILD_SUBDIRS):
	@$(MKDIR) $@

$(NAME): $(OBJS)
	@echo "Linking $(NAME)..."
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@
	@echo "Build complete!"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | directories
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning build files..."
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(NAME)

-include $(DEPS)