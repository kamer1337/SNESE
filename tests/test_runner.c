/*
 * test_runner.c - Main test runner
 * 
 * Runs all unit tests for SNESE project
 */

#include "test_framework.h"
#include <stdlib.h>

/* Declare test suite functions */
void test_cartridge_suite(void);
void test_script_suite(void);

int main(void) {
    test_init();
    
    /* Run all test suites */
    test_cartridge_suite();
    test_script_suite();
    
    /* Print summary */
    test_summary();
    
    /* Return exit code based on results */
    return (g_test_stats.tests_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
