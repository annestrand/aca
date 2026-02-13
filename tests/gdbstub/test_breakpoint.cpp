#include <iostream>
#include <signal.h>
#include <string>
#include <vector>

#include "test_common.hpp"
#include "gtest/gtest.h"

#include "aca_gdbstub.h"

TEST(gdbstub, test_set_soft_breakpoint) {
    // Create mock test packet
    const char *packet = "$Z0,d8,4#b2";

    // Create mock putchar buff
    std::vector<char> dummyPutchar;
    g_putcharPktHandle = &dummyPutchar;

    TestBreak           breakObj   = {{0}};
    aca_gdbstub_context gdbstubCtx = {0};
    gdbstubCtx.usrData             = &breakObj;
    aca_gdb_packet gdbPkt;
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInit(&gdbPkt.pktData, 32));
    gdbPkt.commandType = 'Z';
    gdbPkt.checksum[0] = 'b';
    gdbPkt.checksum[1] = '2';
    gdbPkt.checksum[2] = 0;
    for (size_t i = 0; i < strlen(packet); ++i) {
        GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&gdbPkt.pktData, packet[i]));
    }
    acaGdbstubProcessBreakpoint(&gdbstubCtx, &gdbPkt, ACA_GDBSTUB_SET_BREAKPOINT);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);

    TestBreak *brkObj = (TestBreak *)gdbstubCtx.usrData;
    EXPECT_EQ(brkObj->addr, 0xd8U);
    EXPECT_EQ(brkObj->config.softBreak, 1U);
    EXPECT_EQ(brkObj->config.isSet, 1U);
    EXPECT_EQ(brkObj->config.hardBreak, 0U);
    EXPECT_EQ(brkObj->config.isClear, 0U);

    acaDynamicCharBufferFree(&gdbPkt.pktData);
}
