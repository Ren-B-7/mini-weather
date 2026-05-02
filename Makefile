# Project Name (edit this)
TARGET_NAME = weather
BUILD_DIR = build
BIN_DIR = bin
EXECUTABLE = $(BIN_DIR)/$(TARGET_NAME)
INSTALL_DIR = $(HOME)/.local/bin

# Source files (edit these) - MUST be in src/ directory
SRCS = src/main.c src/example.c src/api.c
# Header files (edit these if you add headers outside src/include/)
HDRS = src/include/example.h

# Object files (placed in build/ directory)
OBJS = $(SRCS:src/%.c=$(BUILD_DIR)/%.o)

# Compiler and flags
CC = gcc
# Comprehensive CFLAGS
CFLAGS = -std=c11 -pedantic -Wall -Wextra -Werror -Wformat=2 -Wshadow -Wconversion -Wsign-conversion -Wundef -Wstrict-prototypes -Wmissing-prototypes -Wredundant-decls -Wpointer-arith -Wwrite-strings -Wold-style-definition -Isrc/include
# Hardening flags
HARDENING = -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -fstack-clash-protection -fcf-protection
# Optimization flags
OPTFLAGS = -O3 -march=native -flto

# Linker flags
LDFLAGS = -lm -pie -Wl,-z,relro,-z,now -lcjson -lcurl

# Combine all flags
ALL_CFLAGS = $(CFLAGS) $(HARDENING) $(OPTFLAGS)

# Targets
.PHONY: all clean run format lint directories install uninstall

all: directories $(EXECUTABLE)

# Create output directories if they don't exist
directories:
	@mkdir -p $(BIN_DIR) $(BUILD_DIR)

# Rule to compile .c files into .o files in the build/ directory
$(BUILD_DIR)/%.o: src/%.c
	@echo "Compiling $< ..."
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# Rule to link the executable in the bin/ directory
$(EXECUTABLE): $(OBJS)
	@echo "Linking $@ ..."
	$(CC) $(ALL_CFLAGS) $(LDFLAGS) -o $(EXECUTABLE) $(OBJS)

# Rule to clean up build artifacts
clean:
	@echo "Cleaning up build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

# Rule to run the executable
run: $(EXECUTABLE)
	@echo "Running $(EXECUTABLE) ..."
	./$(EXECUTABLE)

install: all
	@echo "Installing $(TARGET_NAME) to $(INSTALL_DIR) ..."
	@mkdir -p $(INSTALL_DIR)
	@install -m 755 $(EXECUTABLE) $(INSTALL_DIR)
	@echo "Installed $(TARGET_NAME) to $(INSTALL_DIR)"

uninstall:
	@echo "Uninstalling $(TARGET_NAME) from $(INSTALL_DIR) ..."
	@rm -f $(INSTALL_DIR)/$(TARGET_NAME)
	@echo "Uninstalled $(TARGET_NAME)"

# Format code using clang-format
FORMAT_FILES = $(SRCS) $(HDRS)
format:
	@echo "Formatting code..."
	@clang-format -style=file:./.clang-format -i $(FORMAT_FILES)
	mbake format --config ./.bake.toml Makefile

# Run static analysis with clang-tidy
CLANG_TIDY_CHECKS = -checks=-*,readability-braces-around-statements,bugprone-argument-comment,readability-simplify-boolean-expr,bugprone-suspicious-missing-comma,bugprone-unused-raii,readability-else-after-return,performance-no-int-to-ptr,bugprone-bool-pointer-implicit-conversion,bugprone-integer-division,bugprone-macro-parentheses,readability-uppercase-literal-suffix,performance-unnecessary-copy-initialization
lint:
	@echo "Running static analysis..."
	@clang-tidy $(CLANG_TIDY_CHECKS) -header-filter='^(?!.*).*$$' $(SRCS) -- $(CFLAGS)
	mbake validate --config ./.bake.toml Makefile
