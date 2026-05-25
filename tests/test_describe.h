#pragma once

#include <cstdio>
#include <cstdlib>
#include <gtest/gtest.h>

/* DESCRIBE("text") — inline test documentation.
 *
 * Always: stores the description as a property in XML/JSON output.
 * Verbose mode: also prints to stdout when GTEST_DESCRIBE=1 is set.
 *
 * Usage:
 *   TEST(Suite, Name) {
 *       DESCRIBE("what this test verifies and why");
 *       ...
 *   }
 *
 * Normal run:   ./test_cli               — silent, clean output
 * Verbose run:  GTEST_DESCRIBE=1 ./test_cli — descriptions printed per test
 * Structured:   ./test_cli --gtest_output=json:out.json — descriptions in JSON
 */
inline void test_describe(const char *desc) {
    ::testing::Test::RecordProperty("description", desc);
    if (std::getenv("GTEST_DESCRIBE")) {
        std::printf("--%s\n", desc);
    }
}

#define DESCRIBE(desc) test_describe(desc)
