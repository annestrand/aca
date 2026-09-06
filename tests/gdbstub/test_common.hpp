#ifndef MINIGDBSTUB_TEST_COMMON_HPP
#define MINIGDBSTUB_TEST_COMMON_HPP

#include <iostream> // IWYU pragma: keep
#include <vector>

#define GTEST_COUT std::cerr << "\033[0;32m[ INFO     ] \033[0;37m"
#define GTEST_FAIL_IF_ERR(x)                                                                       \
    if ((x) != ACA_GDBSTUB_SUCCESS) {                                                              \
        FAIL() << #x << " != ACA_GDBSTUB_SUCCESS";                                                 \
    }

struct test_break {
    struct config {
        unsigned int softBreak : 1;
        unsigned int hardBreak : 1;
        unsigned int isSet : 1;
        unsigned int isClear : 1;
    };
    config config;
    size_t addr;
};

// Test globals
extern std::vector<char>          *pGetcharPktHandle, *pPutcharPktHandle;
extern std::vector<unsigned char> *pMemHandle;
extern int                         gGetcharPktIndex;

#endif // MINIGDBSTUB_TEST_COMMON_HPP