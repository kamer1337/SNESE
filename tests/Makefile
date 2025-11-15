# Makefile for SNESE test suite

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -I../include -Iinclude
LDFLAGS = 
LIBS = 

# Directories
SRC_DIR = ../src
INC_DIR = ../include
BUILD_DIR = build
BIN_DIR = .

# Target executable
TARGET = $(BIN_DIR)/test_runner

# Source files for main project (exclude main.c)
PROJECT_SOURCES = $(SRC_DIR)/cartridge.c $(SRC_DIR)/memory.c $(SRC_DIR)/script.c
PROJECT_OBJECTS = $(PROJECT_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Test source files
TEST_SOURCES = test_runner.c test_framework.c test_cartridge.c test_script.c
TEST_OBJECTS = $(TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)

# All objects
ALL_OBJECTS = $(PROJECT_OBJECTS) $(TEST_OBJECTS)

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link test runner
$(TARGET): $(BUILD_DIR) $(ALL_OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(LDFLAGS) $(ALL_OBJECTS) $(LIBS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Compile project source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test source files
$(BUILD_DIR)/%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test: $(TARGET)
	@echo ""
	@echo "Running tests..."
	@./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning test artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Print build information
info:
	@echo "Test Suite Build Information"
	@echo "============================="
	@echo "CC:              $(CC)"
	@echo "CFLAGS:          $(CFLAGS)"
	@echo "PROJECT_SOURCES: $(PROJECT_SOURCES)"
	@echo "TEST_SOURCES:    $(TEST_SOURCES)"
	@echo "TARGET:          $(TARGET)"

.PHONY: all test clean info
