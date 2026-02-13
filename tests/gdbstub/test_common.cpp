#include "test_common.hpp"

// Test globals
std::vector<char>          *g_getcharPktHandle = nullptr, *g_putcharPktHandle = nullptr;
std::vector<unsigned char> *g_memHandle       = nullptr;
int                         g_getcharPktIndex = 0;

// Test user handlers - these are used by the aca_gdbstub library to handle interactions with the
// target system (e.g. reading/writing memory, continuing execution, etc.) - these are mocked for
// testing purposes in aca_gdbstub tests

void acaGdbstubPutcharStub(char c, void *usrData) {
    g_putcharPktHandle->push_back(c);
    return;
}

char acaGdbstubGetcharStub(void *usrData) {
    return (*g_getcharPktHandle)[g_getcharPktIndex++];
}

unsigned char acaGdbstubReadMemStub(size_t addr, void *usrData) {
    return (*g_memHandle)[addr];
}

void acaGdbstubWriteMemStub(size_t addr, unsigned char data, void *usrData) {
    (*g_memHandle)[addr] = data;
    return;
}

void acaGdbstubContinueStub(void *usrData) {
    return;
}

void acaGdbstubStepStub(void *usrData) {
    return;
}

void acaGdbstubProcessBreakpointStub(int type, size_t addr, void *usrData) {
    TestBreak *brkObj = (TestBreak *)usrData;
    brkObj->addr      = addr;

    if (type & ACA_GDBSTUB_HARD_BREAKPOINT)
        brkObj->config.hardBreak = 1;
    if (type & ACA_GDBSTUB_SOFT_BREAKPOINT)
        brkObj->config.softBreak = 1;
    if (type & ACA_GDBSTUB_CLEAR_BREAKPOINT)
        brkObj->config.isClear = 1;
    if (type & ACA_GDBSTUB_SET_BREAKPOINT)
        brkObj->config.isSet = 1;
    return;
}

void acaGdbstubKillSessionStub(void *usrData) {
    return;
}
