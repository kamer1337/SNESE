# Makefile for SNESE - SNES Emulator with Built-in Game Maker
# Compatible with GCC 15.2+ on Windows 11 x64 and Linux

# Compiler settings
CC ?= gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

# Platform detection
ifeq ($(OS),Windows_NT)
    # Windows-specific settings
    PLATFORM = windows
    TARGET_EXT = .exe
    RM = del /Q
    RMDIR = rmdir /S /Q
    MKDIR = mkdir
    PATHSEP = \\
    # Disable pedantic on Windows to avoid POSIX warnings
    CFLAGS += -D_WIN32 -DWIN32
else
    # Linux/Unix settings
    PLATFORM = linux
    TARGET_EXT =
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p
    PATHSEP = /
    CFLAGS += -pedantic
endif

LDFLAGS = 
LIBS = -lm

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = .

# Target executable
TARGET = $(BIN_DIR)/snesemu$(TARGET_EXT)

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
HEADERS = $(wildcard $(INC_DIR)/*.h)

# Build configurations
DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2
endif

# Default target - builds only the emulator (tests excluded by default)
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
ifeq ($(PLATFORM),windows)
	@if not exist $(BUILD_DIR) $(MKDIR) $(BUILD_DIR)
else
	$(MKDIR) $(BUILD_DIR)
endif

# Link object files to create executable
$(TARGET): $(BUILD_DIR) $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
ifeq ($(PLATFORM),windows)
	@if exist $(BUILD_DIR) $(RMDIR) $(BUILD_DIR)
	@if exist $(TARGET) $(RM) $(TARGET)
else
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(TARGET)
endif
	@echo "Clean complete"

# Install (optional - copies to /usr/local/bin) - Linux only
install: $(TARGET)
ifeq ($(PLATFORM),windows)
	@echo "Install target not supported on Windows"
else
	@echo "Installing $(TARGET) to /usr/local/bin..."
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installation complete"
endif

# Uninstall - Linux only
uninstall:
ifeq ($(PLATFORM),windows)
	@echo "Uninstall target not supported on Windows"
else
	@echo "Uninstalling..."
	rm -f /usr/local/bin/snesemu
	@echo "Uninstall complete"
endif

# Run with debug flags
debug: DEBUG=1
debug: clean all

# Print build information
info:
	@echo "SNESE Build Information"
	@echo "======================="
	@echo "Platform: $(PLATFORM)"
	@echo "CC:       $(CC)"
	@echo "CFLAGS:   $(CFLAGS)"
	@echo "LDFLAGS:  $(LDFLAGS)"
	@echo "LIBS:     $(LIBS)"
	@echo "SOURCES:  $(SOURCES)"
	@echo "OBJECTS:  $(OBJECTS)"
	@echo "TARGET:   $(TARGET)"

# Help target
help:
	@echo "SNESE Makefile targets:"
	@echo "  all      - Build the emulator (default, tests excluded)"
	@echo "  clean    - Remove build artifacts"
	@echo "  debug    - Build with debug symbols"
	@echo "  test     - Build and run test suite (optional)"
	@echo "  install  - Install to /usr/local/bin (Linux only)"
	@echo "  uninstall- Remove from /usr/local/bin (Linux only)"
	@echo "  info     - Display build configuration"
	@echo "  help     - Display this help message"
	@echo ""
	@echo "Build options:"
	@echo "  DEBUG=1  - Enable debug build"
	@echo ""
	@echo "Example usage:"
	@echo "  make              # Build release version"
	@echo "  make DEBUG=1      # Build debug version"
	@echo "  make clean all    # Clean rebuild"
	@echo "  make test         # Build and run tests (optional)"

# Phony targets
.PHONY: all clean install uninstall debug info help test test-clean

# Test target - only builds tests when explicitly requested
test:
	@echo "Building and running test suite..."
ifeq ($(PLATFORM),windows)
	@cd tests && $(MAKE) test
else
	@cd tests && $(MAKE) test
endif

# Clean tests
test-clean:
ifeq ($(PLATFORM),windows)
	@cd tests && $(MAKE) clean
else
	@cd tests && $(MAKE) clean
endif
