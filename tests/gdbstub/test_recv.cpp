#include <iostream>
#include <signal.h>
#include <string>
#include <vector>

#include "test_common.hpp"
#include "gtest/gtest.h"

#include "aca_gdbstub.h"

// --- Tests ---

TEST(gdbstub, test_recvs) {
    // Create mock test packets
    std::vector<const char *> mockPackets = {"+$g#67", "+$G#47", "+$c#63", "+$Ga700467f#46"};

    for (auto packet : mockPackets) {
        // Create getchar buffer
        std::vector<char> testBuff;
        for (size_t i = 0; i < strlen(packet); ++i) {
            testBuff.push_back(packet[i]);
        }
        g_getcharPktHandle = &testBuff;
        g_getcharPktIndex  = 0;

        // Create putchar buffer
        std::vector<char> testBuff2;
        g_putcharPktHandle = &testBuff2;

        aca_gdbstub_context gdbstubCtx = {0};
        aca_gdb_packet      gdbPkt     = {0};
        GTEST_FAIL_IF_ERR(acaDynamicCharBufferInit(&gdbPkt.pktData, ACA_GDBSTUB_PKT_SIZE));

        acaGdbstubRecv(&gdbstubCtx, &gdbPkt);
        GTEST_FAIL_IF_ERR(gdbstubCtx.err);

        EXPECT_EQ(gdbPkt.commandType, packet[2]);
        acaDynamicCharBufferFree(&gdbPkt.pktData);
    }
}