# Makefile for SNESE - SNES Emulator with Built-in Game Maker

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -Iinclude
LDFLAGS = 
LIBS = 

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = .

# Target executable
TARGET = $(BIN_DIR)/snesemu

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

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

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
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Install (optional - copies to /usr/local/bin)
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling..."
	rm -f /usr/local/bin/snesemu
	@echo "Uninstall complete"

# Run with debug flags
debug: DEBUG=1
debug: clean all

# Print build information
info:
	@echo "SNESE Build Information"
	@echo "======================="
	@echo "CC:       $(CC)"
	@echo "CFLAGS:   $(CFLAGS)"
	@echo "LDFLAGS:  $(LDFLAGS)"
	@echo "SOURCES:  $(SOURCES)"
	@echo "OBJECTS:  $(OBJECTS)"
	@echo "TARGET:   $(TARGET)"

# Help target
help:
	@echo "SNESE Makefile targets:"
	@echo "  all      - Build the emulator (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  debug    - Build with debug symbols"
	@echo "  test     - Run test suite"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  uninstall- Remove from /usr/local/bin"
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
	@echo "  make test         # Run tests"

# Phony targets
.PHONY: all clean install uninstall debug info help test

# Test target
test:
	@echo "Running test suite..."
	@cd tests && $(MAKE) test

# Clean tests
test-clean:
	@cd tests && $(MAKE) clean
