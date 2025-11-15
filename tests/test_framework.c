/*
 * test_framework.c - Simple unit testing framework implementation
 */

#include "test_framework.h"
#include <string.h>

/* Global test statistics */
TestStats g_test_stats = {0};

void test_init(void) {
    memset(&g_test_stats, 0, sizeof(g_test_stats));
    printf("\n╔═══════════════════════════════════════════════════════╗\n");
    printf("║           SNESE Test Suite                           ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
}

void test_summary(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║           Test Summary                               ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Total tests run:    %d\n", g_test_stats.tests_run);
    printf("Tests passed:       %d\n", g_test_stats.tests_passed);
    printf("Tests failed:       %d\n", g_test_stats.tests_failed);
    printf("\n");
    
    if (g_test_stats.tests_failed == 0) {
        printf("Result: ✓ ALL TESTS PASSED\n");
    } else {
        printf("Result: ✗ SOME TESTS FAILED\n");
    }
    printf("\n");
}
