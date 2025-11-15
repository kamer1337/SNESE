# SNESE Test Suite

This directory contains unit tests and integration tests for the SNESE emulator project.

## Running Tests

To build and run all tests:

```bash
# From the project root
make test

# Or from the tests directory
cd tests
make test
```

## Test Structure

- `test_framework.h/c` - Minimal testing framework with assertion macros
- `test_cartridge.c` - Tests for the cartridge/ROM loading module
- `test_script.c` - Tests for the scripting engine
- `test_runner.c` - Main test runner that executes all test suites

## Writing New Tests

1. Create a new test file (e.g., `test_mymodule.c`)
2. Include `test_framework.h`
3. Write test functions using the provided macros:

```c
void test_my_feature(void) {
    TEST("Description of test");
    
    // Your test code here
    ASSERT(condition);
    ASSERT_EQ(actual, expected);
    ASSERT_STR_EQ(str1, str2);
    
    TEST_PASS();
}

void test_mymodule_suite(void) {
    TEST_SUITE("My Module");
    
    test_my_feature();
    test_another_feature();
}
```

4. Add your test suite to `test_runner.c`
5. Update the `Makefile` to include your test file

## Test Macros

- `TEST_SUITE(name)` - Start a new test suite
- `TEST(name)` - Start a new test
- `ASSERT(condition)` - Assert that condition is true
- `ASSERT_EQ(a, b)` - Assert that a equals b
- `ASSERT_STR_EQ(a, b)` - Assert that strings a and b are equal
- `TEST_PASS()` - Mark test as passed

## Current Test Coverage

### Cartridge Module
- ROM initialization
- ROM data writing
- Backup and restore functionality
- Checksum calculation

### Script Module
- Command parsing (SET, SET16, FILL, COPY, comments, labels)
- Command execution
- Script execution from strings and files

## Future Tests

- PPU rendering tests
- CPU instruction tests
- Memory mapping tests
- Input system tests
- Game Maker editor tests
- Integration tests with actual ROM files

## Notes

- Tests use minimal dependencies to keep build times fast
- No external test frameworks required
- All tests should be deterministic and isolated
- Test ROMs should not be committed to the repository
