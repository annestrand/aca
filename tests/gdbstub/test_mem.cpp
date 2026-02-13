#include <algorithm>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>

#include "test_common.hpp"
#include "gtest/gtest.h"

#include "aca_gdbstub.h"

TEST(gdbstub, test_m) {
    aca_gdb_packet      mockPkt    = {0};
    aca_gdbstub_context gdbstubCtx = {0};
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInit(&mockPkt.pktData, 32));

    // Create mock memory
    std::vector<unsigned char> dummyMem(128);
    dummyMem[8]  = 'c';
    dummyMem[9]  = 'o';
    dummyMem[10] = 'o';
    dummyMem[11] = 'l';
    g_memHandle  = &dummyMem;

    // Create mock putchar buff
    std::vector<char> dummyPutchar;
    g_putcharPktHandle = &dummyPutchar;

    // Read 4 bytes starting at address 0x8
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'm'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '8'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, ','));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '4'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 0));
    acaGdbstubReadMem(&gdbstubCtx, &mockPkt);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);

    // Verify putchar buffer matches the dummyMem
    for (int i = 0; i < 4; ++i) {
        char itoaBuff[3];
        ACA_GDBSTUB_HEX_ENCODE_ASCII(dummyMem[i + 8], 3, itoaBuff);
        // Swap if single digit
        if (itoaBuff[1] == 0) {
            itoaBuff[1] = itoaBuff[0];
            itoaBuff[0] = '0';
        }
        EXPECT_EQ(itoaBuff[0], (*g_putcharPktHandle)[(i * 2) + 1]);
        EXPECT_EQ(itoaBuff[1], (*g_putcharPktHandle)[(i * 2) + 2]);
    }
    acaDynamicCharBufferFree(&mockPkt.pktData);
}

TEST(gdbstub, test_M) {
    aca_gdbstub_context gdbstubCtx = {0};
    aca_gdb_packet      mockPkt    = {0};
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInit(&mockPkt.pktData, 32));

    // Create mock memory
    std::vector<unsigned char> dummyMem(128);
    g_memHandle = &dummyMem;

    // Create mock putchar handle
    std::vector<char> putcharHandle;
    g_putcharPktHandle = &putcharHandle;

    // Write '0xdeadbeef' starting at address 0x4
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'M'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '4'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, ','));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, '4'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, ':'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'd'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'e'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'a'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'd'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'b'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'e'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'e'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 'f'));
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&mockPkt.pktData, 0));

    acaGdbstubWriteMem(&gdbstubCtx, &mockPkt);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);

    const unsigned char expectedValue[] = {(unsigned char)222,
                                           (unsigned char)173,
                                           (unsigned char)190,
                                           (unsigned char)239,
                                           (unsigned char)0};
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(dummyMem[4 + i], expectedValue[i]);
    }
}