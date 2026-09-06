#include "aca_gdbstub.h"
#include "gtest/gtest.h"
#include "test_common.hpp"

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

TEST(gdbstub, test_set_soft_breakpoint) {
    // Create mock test packet
    const char *pAcket = "$Z0,d8,4#b2";

    // Create mock putchar buff
    std::vector<char> dummyPutchar;
    pPutcharPktHandle = &dummyPutchar;

    test_break          breakObj   = {{0}};
    aca_gdbstub_context gdbstubCtx = {0};
    gdbstubCtx.usrData             = &breakObj;
    aca_gdb_packet gdbPkt;
    GTEST_FAIL_IF_ERR(acaDynamicCharBufferInit(&gdbPkt.pktData, 32));
    gdbPkt.commandType = 'Z';
    gdbPkt.checksum[0] = 'b';
    gdbPkt.checksum[1] = '2';
    gdbPkt.checksum[2] = 0;
    for (size_t i = 0; i < strlen(pAcket); ++i) {
        GTEST_FAIL_IF_ERR(acaDynamicCharBufferInsert(&gdbPkt.pktData, pAcket[i]));
    }
    acaGdbstubProcessBreakpoint(&gdbstubCtx, &gdbPkt, ACA_GDBSTUB_SET_BREAKPOINT);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);

    test_break *pBrkObj = (test_break *)gdbstubCtx.usrData;
    EXPECT_EQ(pBrkObj->addr, 0xd8U);
    EXPECT_EQ(pBrkObj->config.softBreak, 1U);
    EXPECT_EQ(pBrkObj->config.isSet, 1U);
    EXPECT_EQ(pBrkObj->config.hardBreak, 0U);
    EXPECT_EQ(pBrkObj->config.isClear, 0U);

    acaDynamicCharBufferFree(&gdbPkt.pktData);
}
