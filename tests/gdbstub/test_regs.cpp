#include <algorithm>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>

#include "test_common.hpp"
#include "gtest/gtest.h"

#include "aca_gdbstub.h"

TEST(gdbstub, test_g) {
    // Create dummy regfile
    int                 regs[8]    = {2, 4, 55, 6, 12, 23, 81, 1};
    size_t              regSize    = sizeof(regs);
    aca_gdbstub_context gdbstubCtx = {0};
    gdbstubCtx.regs                = (char *)regs;
    gdbstubCtx.regsSize            = regSize;
    gdbstubCtx.regsCount           = 8;
    gdbstubCtx.signalNum           = 5;

    std::vector<char> testVec;
    g_putcharPktHandle = &testVec;
    g_putcharPktHandle->clear();
    acaGdbstubSendRegs(&gdbstubCtx);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);

    for (size_t i = 0; i < regSize; ++i) {
        char itoaBuff[3];
        ACA_GDBSTUB_HEX_ENCODE_ASCII(gdbstubCtx.regs[i], 3, itoaBuff);
        // Swap if single digit
        if (itoaBuff[1] == 0) {
            itoaBuff[1] = itoaBuff[0];
            itoaBuff[0] = '0';
        }
        EXPECT_EQ(itoaBuff[0], (*g_putcharPktHandle)[(i * 2) + 1]);
        EXPECT_EQ(itoaBuff[1], (*g_putcharPktHandle)[(i * 2) + 2]);
    }
}

TEST(gdbstub, test_G) {
    char charRegs[(2 * 8 * sizeof(int)) + 1] = {
        '0', 'b', '0', '0', '0', '0', '0', '0', // regs[0] = 11
        '0', '4', '0', '0', '0', '0', '0', '0', // regs[1] = 4
        '0', '5', '0', '0', '0', '0', '0', '0', // regs[2] = 5
        '0', '6', '0', '0', '0', '0', '0', '0', // regs[3] = 6
        '3', '7', '0', '0', '0', '0', '0', '0', // regs[4] = 55
        '2', '2', '0', '0', '0', '0', '0', '0', // regs[5] = 34
        '1', '7', '0', '0', '0', '0', '0', '0', // regs[6] = 23
        '1', '0', '0', '0', '0', '0', '0', '0', // regs[7] = 16
        0};
    int expectedResults[8] = {11, 4, 5, 6, 55, 34, 23, 16};
    int regs2[8]           = {1, 1, 1, 1, 1, 1, 1, 1};

    aca_gdb_packet recvPkt;
    recvPkt.pktData.buffer = charRegs;
    recvPkt.pktData.size   = sizeof(charRegs);

    aca_gdbstub_context procObj = {0};
    procObj.regs                = (char *)regs2;

    acaGdbstubWriteRegs(&procObj, &recvPkt);
    GTEST_FAIL_IF_ERR(procObj.err);
    for (size_t i = 0; i < sizeof(expectedResults) / sizeof(int); i++) {
        EXPECT_EQ(expectedResults[i], regs2[i]);
    }
}

TEST(gdbstub, test_p) {
    aca_gdb_packet      mockPkt;
    aca_gdbstub_context gdbstubCtx = {0};
    int                 regs[8]    = {1, 1, 1, 1, 1, 50, 1, 1};
    gdbstubCtx.regs                = (char *)regs;
    gdbstubCtx.regsSize            = sizeof(regs);
    gdbstubCtx.regsCount           = 8;
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInit(&mockPkt.pktData, 32));

    // Read register at index 6
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '$'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'p'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '5'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '#'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'a'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '5'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 0));

    std::vector<char> testVec;
    g_putcharPktHandle = &testVec;
    g_putcharPktHandle->clear();

    acaGdbstubSendReg(&gdbstubCtx, &mockPkt);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);
    int actualResult = 0;
    int rotL         = 1;
    std::rotate(testVec.begin(), testVec.begin() + rotL, testVec.end());
    auto it      = std::find(testVec.begin(), testVec.end(), '#');
    *it          = 0;
    actualResult = strtol(testVec.data(), NULL, 16);
    int byte0    = actualResult & 0xff;
    int byte1    = (actualResult & 0xff00) >> 8;
    int byte2    = (actualResult & 0xff0000) >> 8 * 2;
    int byte3    = (actualResult & 0xff000000) >> 8 * 3;
    actualResult = (byte0 << 8 * 3) | (byte1 << 8 * 2) | (byte2 << 8 * 1) | byte3;
    EXPECT_EQ(regs[5], actualResult);
    acaDynamicCharBufferFree(&mockPkt.pktData);
}

TEST(gdbstub, test_P) {
    aca_gdb_packet      mockPkt;
    aca_gdbstub_context gdbstubCtx = {0};
    int                 regs[8]    = {1, 1, 1, 1, 1, 1, 1, 1};
    gdbstubCtx.regs                = (char *)regs;
    gdbstubCtx.regsSize            = sizeof(regs);
    gdbstubCtx.regsCount           = 8;
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInit(&mockPkt.pktData, 32));

    // Write '23' at register index 3
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'P'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '3'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '='));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '1'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '7'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 0));

    acaGdbstubWriteReg(&gdbstubCtx, &mockPkt);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);
    EXPECT_EQ(regs[3], 23);
    acaDynamicCharBufferFree(&mockPkt.pktData);
}