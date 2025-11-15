/*
 * test_framework.h - Simple unit testing framework
 * 
 * Minimal testing framework for SNESE project
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>

/* Test statistics */
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
    const char *current_suite;
} TestStats;

/* Global test statistics */
extern TestStats g_test_stats;

/* Test macros */
#define TEST_SUITE(name) \
    do { \
        g_test_stats.current_suite = name; \
        printf("\n=== Test Suite: %s ===\n", name); \
    } while (0)

#define TEST(name) \
    do { \
        g_test_stats.tests_run++; \
        printf("  Running: %s ... ", name); \
        fflush(stdout); \
    } while (0)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("FAIL\n"); \
            printf("    Assertion failed: %s\n", #condition); \
            printf("    File: %s, Line: %d\n", __FILE__, __LINE__); \
            g_test_stats.tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            printf("FAIL\n"); \
            printf("    Expected: %d, Got: %d\n", (int)(b), (int)(a)); \
            printf("    File: %s, Line: %d\n", __FILE__, __LINE__); \
            g_test_stats.tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf("FAIL\n"); \
            printf("    Expected: \"%s\", Got: \"%s\"\n", (b), (a)); \
            printf("    File: %s, Line: %d\n", __FILE__, __LINE__); \
            g_test_stats.tests_failed++; \
            return; \
        } \
    } while (0)

#define TEST_PASS() \
    do { \
        printf("PASS\n"); \
        g_test_stats.tests_passed++; \
    } while (0)

/* Initialize test framework */
void test_init(void);

/* Print test summary */
void test_summary(void);

#endif /* TEST_FRAMEWORK_H */
