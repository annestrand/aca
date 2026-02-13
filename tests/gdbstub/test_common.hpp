#ifndef MINIGDBSTUB_TEST_COMMON_HPP
#define MINIGDBSTUB_TEST_COMMON_HPP

#include "aca_gdbstub.h"

#include <iostream>
#include <vector>

#define GTEST_COUT std::cerr << "\033[0;32m[ INFO     ] \033[0;37m"
#define GTEST_FAIL_IF_ERR(x)                                                                       \
    if (x != ACA_GDBSTUB_SUCCESS) {                                                                \
        FAIL() << #x << " != ACA_GDBSTUB_SUCCESS";                                                 \
    }

struct TestBreak {
    struct Config {
        unsigned int softBreak : 1;
        unsigned int hardBreak : 1;
        unsigned int isSet : 1;
        unsigned int isClear : 1;
    };
    Config config;
    size_t addr;
};

// Test globals
extern std::vector<char>          *g_getcharPktHandle, *g_putcharPktHandle;
extern std::vector<unsigned char> *g_memHandle;
extern int                         g_getcharPktIndex;

// Test user handlers - these are used by the aca_gdbstub library to handle interactions with the
// target system (e.g. reading/writing memory, continuing execution, etc.) - these are mocked for
// testing purposes in aca_gdbstub tests
void          acaGdbstubPutcharStub(char c, void *usrData);
char          acaGdbstubGetcharStub(void *usrData);
unsigned char acaGdbstubReadMemStub(size_t addr, void *usrData);
void          acaGdbstubWriteMemStub(size_t addr, unsigned char data, void *usrData);
void          acaGdbstubContinueStub(void *usrData);
void          acaGdbstubStepStub(void *usrData);
void          acaGdbstubProcessBreakpointStub(int type, size_t addr, void *usrData);
void          acaGdbstubKillSessionStub(void *usrData);

#endif // MINIGDBSTUB_TEST_COMMON_HPP