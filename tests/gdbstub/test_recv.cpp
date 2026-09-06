#include "aca_gdbstub.h"
#include "gtest/gtest.h"
#include "test_common.hpp"

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

// --- Tests ---

TEST(gdbstub, test_recvs) {
    // Create mock test packets
    std::vector<const char *> mockPackets = {"+$g#67", "+$G#47", "+$c#63", "+$Ga700467f#46"};

    for (const char *pPacket : mockPackets) {
        // Create getchar buffer
        std::vector<char> testBuff;
        testBuff.reserve(strlen(pPacket));
        for (size_t i = 0; i < strlen(pPacket); ++i) {
            testBuff.push_back(pPacket[i]);
        }
        pGetcharPktHandle = &testBuff;
        gGetcharPktIndex  = 0;

        // Create putchar buffer
        std::vector<char> testBuff2;
        pPutcharPktHandle = &testBuff2;

        aca_gdbstub_context gdbstubCtx = {0};
        aca_gdb_packet      gdbPkt     = {0};
        GTEST_FAIL_IF_ERR(acaDynamicCharBufferInit(&gdbPkt.pktData, 64));

        acaGdbstubRecv(&gdbstubCtx, &gdbPkt);
        GTEST_FAIL_IF_ERR(gdbstubCtx.err);

        EXPECT_EQ(gdbPkt.commandType, pPacket[2]);
        acaDynamicCharBufferFree(&gdbPkt.pktData);
    }
}