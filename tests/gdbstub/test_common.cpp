#include "test_common.hpp"

#include "aca_gdbstub.h"

#include <cstddef>
#include <vector>

// Test globals
std::vector<char>          *pGetcharPktHandle = nullptr, *pPutcharPktHandle = nullptr;
std::vector<unsigned char> *pMemHandle       = nullptr;
int                         gGetcharPktIndex = 0;

// Test user handlers - these are used by the aca_gdbstub library to handle interactions with the
// target system (e.g. reading/writing memory, continuing execution, etc.) - these are mocked for
// testing purposes in aca_gdbstub tests

void acaGdbstubPutcharStub(char c, void *usrData) {
    pPutcharPktHandle->push_back(c);
    return;
}

char acaGdbstubGetcharStub(void *usrData) {
    return (*pGetcharPktHandle)[gGetcharPktIndex++];
}

unsigned char acaGdbstubReadMemStub(size_t addr, void *usrData) {
    return (*pMemHandle)[addr];
}

void acaGdbstubWriteMemStub(size_t addr, unsigned char data, void *usrData) {
    (*pMemHandle)[addr] = data;
    return;
}

void acaGdbstubContinueStub(void *usrData) {
    return;
}

void acaGdbstubStepStub(void *usrData) {
    return;
}

void acaGdbstubProcessBreakpointStub(int type, size_t addr, void *usrData) {
    test_break *pBrkObj = (test_break *)usrData;
    pBrkObj->addr       = addr;

    if (type & ACA_GDBSTUB_HARD_BREAKPOINT) {
        pBrkObj->config.hardBreak = 1;
    }
    if (type & ACA_GDBSTUB_SOFT_BREAKPOINT) {
        pBrkObj->config.softBreak = 1;
    }
    if (type & ACA_GDBSTUB_CLEAR_BREAKPOINT) {
        pBrkObj->config.isClear = 1;
    }
    if (type & ACA_GDBSTUB_SET_BREAKPOINT) {
        pBrkObj->config.isSet = 1;
    }
    return;
}

void acaGdbstubKillSessionStub(void *usrData) {
    return;
}
