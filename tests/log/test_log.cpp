#include <iostream>
#include <signal.h>
#include <string>
#include <vector>

#include "aca_log.h"
#include "gtest/gtest.h"

#if 0 // set/enable for debugging aid when writing tests
#define PRINT_LOG_STR(str) printf("%s", str.c_str())
#else
#define PRINT_LOG_STR(str)
#endif

#define LOG_TEST(level, expected, fmt, ...)                                                        \
    do {                                                                                           \
        testing::internal::CaptureStdout();                                                        \
        ACA_LOG_##level(fmt, __VA_ARGS__);                                                         \
        std::string captured = testing::internal::GetCapturedStdout();                             \
        PRINT_LOG_STR(captured);                                                                   \
        EXPECT_STREQ(expected, captured.c_str());                                                  \
    } while (false)

// special case where we always expect no output
#define LOG_TEST_NULL(level, fmt, ...) LOG_TEST(level, "", fmt, __VA_ARGS__);

TEST(log, null_handler) {
    acaLogSetHandler(acaLogNullHandler);

    // 1. run through some simple scenarios
    LOG_TEST_NULL(INFO, "Hello World!");
    LOG_TEST_NULL(INFO, "Result is %d", 99 + 1);
    LOG_TEST_NULL(WARN, "A: %f, B: %c, C: %u", 31.456, 'b', 555);

    // 2. multi-Level tests
    // note: assuming DEBUG is 5 chars, it usually doesn't need a leading space
    LOG_TEST_NULL(DEBUG, "Debug message");
    LOG_TEST_NULL(WARN, "Warning occurred");
    LOG_TEST_NULL(ERROR, "Error code: %d", 500);

    // 3. edge cases: empty strings and percent signs
    LOG_TEST_NULL(INFO, "%s", "");
    LOG_TEST_NULL(INFO, "Progress: %d%%", 100);

    // 4. formatting and types
    LOG_TEST_NULL(INFO, "Hex: 0x%X", 255);
    LOG_TEST_NULL(WARN, "Float: %.2f", 3.14159);

    // 5. large payload test
    std::string longInput(256, 'z');
    std::string expectedOutput = "[ INFO] " + longInput + "\n";
    LOG_TEST_NULL(INFO, "%s", longInput.c_str());
}

TEST(log, basic_handler) {
    acaLogSetHandler(acaLogBasicHandler);

    // 1. run through some simple scenarios
    LOG_TEST(INFO, "[ INFO] Hello World!\n", "Hello World!");
    LOG_TEST(INFO, "[ INFO] Result is 100\n", "Result is %d", 99 + 1);
    LOG_TEST(WARN, "[ WARN] A: 31.456000, B: b, C: 555\n", "A: %f, B: %c, C: %u", 31.456, 'b', 555);

    // 2. multi-Level tests
    // note: assuming DEBUG is 5 chars, it usually doesn't need a leading space
    LOG_TEST(DEBUG, "[DEBUG] Debug message\n", "Debug message");
    LOG_TEST(WARN, "[ WARN] Warning occurred\n", "Warning occurred");
    LOG_TEST(ERROR, "[ERROR] Error code: 500\n", "Error code: %d", 500);

    // 3. edge cases: empty strings and percent signs
    LOG_TEST(INFO, "[ INFO] \n", "%s", "");
    LOG_TEST(INFO, "[ INFO] Progress: 100%\n", "Progress: %d%%", 100);

    // 4. formatting and types
    LOG_TEST(INFO, "[ INFO] Hex: 0xFF\n", "Hex: 0x%X", 255);
    LOG_TEST(WARN, "[ WARN] Float: 3.14\n", "Float: %.2f", 3.14159);

    // 5. large payload test
    std::string longInput(256, 'z');
    std::string expectedOutput = "[ INFO] " + longInput + "\n";
    LOG_TEST(INFO, expectedOutput.c_str(), "%s", longInput.c_str());
}

TEST(log, default_handler) {
    acaLogSetHandler(acaLogStandardHandler);

#define EXPECTED1 "[aca_log_test] [ INFO] [             test_log.cpp:99] Hello World!\n"
#define EXPECTED2 "[aca_log_test] [ INFO] [            test_log.cpp:100] Result is 100\n"
#define EXPECTED3                                                                                  \
    "[aca_log_test] [ WARN] [            test_log.cpp:101] A: 31.456000, B: b, C: 555\n"
#define EXPECTED4 "[aca_log_test] [DEBUG] [            test_log.cpp:105] Debug message\n"
#define EXPECTED5 "[aca_log_test] [ WARN] [            test_log.cpp:106] Warning occurred\n"
#define EXPECTED6 "[aca_log_test] [ERROR] [            test_log.cpp:107] Error code: 500\n"
#define EXPECTED7 "[aca_log_test] [ INFO] [            test_log.cpp:110] \n"
#define EXPECTED8 "[aca_log_test] [ INFO] [            test_log.cpp:111] Progress: 100%\n"
#define EXPECTED9 "[aca_log_test] [ INFO] [            test_log.cpp:114] Hex: 0xFF\n"
#define EXPECTED10 "[aca_log_test] [ WARN] [            test_log.cpp:115] Float: 3.14\n"

    // 1. run through some simple scenarios
    LOG_TEST(INFO, EXPECTED1, "Hello World!");
    LOG_TEST(INFO, EXPECTED2, "Result is %d", 99 + 1);
    LOG_TEST(WARN, EXPECTED3, "A: %f, B: %c, C: %u", 31.456, 'b', 555);

    // 2. multi-Level tests
    // note: assuming DEBUG is 5 chars, it usually doesn't need a leading space
    LOG_TEST(DEBUG, EXPECTED4, "Debug message");
    LOG_TEST(WARN, EXPECTED5, "Warning occurred");
    LOG_TEST(ERROR, EXPECTED6, "Error code: %d", 500);

    // 3. edge cases: empty strings and percent signs
    LOG_TEST(INFO, EXPECTED7, "%s", "");
    LOG_TEST(INFO, EXPECTED8, "Progress: %d%%", 100);

    // 4. formatting and types
    LOG_TEST(INFO, EXPECTED9, "Hex: 0x%X", 255);
    LOG_TEST(WARN, EXPECTED10, "Float: %.2f", 3.14159);

    // 5. large payload test
    std::string longInput(256, 'z');
    std::string expectedOutput =
        "[aca_log_test] [ INFO] [            test_log.cpp:121] " + longInput + "\n";
    LOG_TEST(INFO, expectedOutput.c_str(), "%s", longInput.c_str());
}
