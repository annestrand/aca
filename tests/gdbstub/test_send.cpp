#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <vector>

#include "test_common.hpp"
#include <gtest/gtest.h>

#include "aca_gdbstub.h"

static void cmpCharArrays(char *arr1, char *arr2, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        EXPECT_EQ(arr1[i], arr2[i]) << "Chars " << arr1[i] << " and " << arr2[i] << " differ\n";
    }
}

static void buildSignalPkt(std::stringstream &s, int signal) {
    char checksum[8];
    s << "$S" << std::setw(2) << std::setfill('0') << signal;
    acaGdbstubComputeChecksum(const_cast<char *>(s.str().c_str() + 1), 3, checksum);
    s << "#" << checksum[0] << checksum[1];
}

// --- Tests ---

TEST(gdbstub, basic_signals) {
    // Create signal test-packets
    std::stringstream sigtrapPkt;
    buildSignalPkt(sigtrapPkt, SIGINT);
    std::stringstream sigbusPkt;
    buildSignalPkt(sigbusPkt, SIGILL);

    // Create test putchar buffer
    std::vector<char> testBuff;
    g_putcharPktHandle = &testBuff;

    GTEST_COUT << "    Testing SIGINT...\n";
    aca_gdbstub_context gdbstubCtx = {0};
    gdbstubCtx.signalNum           = SIGINT;
    acaGdbstubSendSignal(&gdbstubCtx);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);
    cmpCharArrays(const_cast<char *>(sigtrapPkt.str().c_str()),
                  testBuff.data(),
                  sizeof(sigtrapPkt.str().c_str()) - 1);
    testBuff.clear();

    GTEST_COUT << "    Testing SIGILL...\n";
    gdbstubCtx.signalNum = SIGILL;
    acaGdbstubSendSignal(&gdbstubCtx);
    GTEST_FAIL_IF_ERR(gdbstubCtx.err);
    cmpCharArrays(const_cast<char *>(sigbusPkt.str().c_str()),
                  testBuff.data(),
                  sizeof(sigbusPkt.str().c_str()) - 1);
    testBuff.clear();
}