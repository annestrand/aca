#include "aca_argparse.h"

#include <gtest/gtest.h>

static aca_argparse_opt_list g_clearOptlist = {ACA_ARGPARSE_HEAD_OPT, NULL};

TEST(argparse, duplicate_options) {
    // Mock argv and argc
    char *argv[] = {(char *)"<EXECUTABLE>",
                    (char *)"-h",
                    (char *)"--data=6",
                    (char *)"--verbose",
                    (char *)"-d",
                    (char *)"7"};
    int   argc   = sizeof(argv) / sizeof(char *);

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    int unknownOpt = acaArgparseParse(argc, argv);
    EXPECT_EQ(unknownOpt, 0);

    // Check if duplicate was found
    EXPECT_EQ(data.infoBits.duplicate, 1U);

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}

TEST(argparse, value_option_at_end_no_value) {
    // Mock argv and argc
    char *argv[] = {(char *)"<EXECUTABLE>", (char *)"-h", (char *)"--verbose", (char *)"-d"};
    int   argc   = sizeof(argv) / sizeof(char *);

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    int unknownOpt = acaArgparseParse(argc, argv);
    EXPECT_EQ(unknownOpt, 0);

    // Check if error was caught
    EXPECT_EQ(data.infoBits.hasErr, 1U);
    EXPECT_TRUE(ACA_ARGPARSE_STR_MATCH(data.errValMsg,
                                       g_aca_argparse_err_strs[ACA_ARGPARSE_ERR_OPT_VAL_END_ARGV]));

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}

TEST(argparse, unknown_option) {
    // Mock argv and argc
    char *argv[] = {(char *)"<EXECUTABLE>",
                    (char *)"--problem",
                    (char *)"-h",
                    (char *)"--verbose",
                    (char *)"--data=8"};
    int   argc   = sizeof(argv) / sizeof(char *);

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    int unknownOpt = acaArgparseParse(argc, argv);

    EXPECT_NE(unknownOpt, 0);
    EXPECT_TRUE(ACA_ARGPARSE_STR_MATCH(argv[unknownOpt], "--problem"));

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}

TEST(argparse, option_value_is_other_option) {
    // Mock argv and argc
    char *argv[] = {(char *)"<EXECUTABLE>",
                    (char *)"--verbose",
                    (char *)"-d",
                    (char *)"-h",
                    (char *)"--data2=8"};
    int   argc   = sizeof(argv) / sizeof(char *);

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    ACA_ARGPARSE_OPT(data2, "", "data2", 1, "Test data2-value option");
    int unknownOpt = acaArgparseParse(argc, argv);
    EXPECT_EQ(unknownOpt, 0);

    // Check if error was caught
    EXPECT_EQ(data.infoBits.hasErr, 1U);
    EXPECT_TRUE(ACA_ARGPARSE_STR_MATCH(data.errValMsg,
                                       g_aca_argparse_err_strs[ACA_ARGPARSE_ERR_VAL_IS_OPT]));

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}

TEST(argparse, option_value_malformed_long_format) {
    // Mock argv and argc
    char *argv[] = {(char *)"<EXECUTABLE>",
                    (char *)"--verbose",
                    (char *)"-d",
                    (char *)"9",
                    (char *)"-h",
                    (char *)"--data2"};
    int   argc   = sizeof(argv) / sizeof(char *);

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    ACA_ARGPARSE_OPT(data2, "", "data2", 1, "Test data2-value option");
    int unknownOpt = acaArgparseParse(argc, argv);
    EXPECT_EQ(unknownOpt, 0);

    // Check if error was caught
    EXPECT_EQ(data2.infoBits.hasErr, 1U);
    EXPECT_TRUE(ACA_ARGPARSE_STR_MATCH(
        data2.errValMsg, g_aca_argparse_err_strs[ACA_ARGPARSE_ERR_MALFORMED_OPT_VAL]));

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}

TEST(argparse, longname_non_value_option_has_value) {
    // Mock argv and argc
    char *argv[] = {(char *)"<EXECUTABLE>",
                    (char *)"--verbose",
                    (char *)"-d",
                    (char *)"9",
                    (char *)"-h",
                    (char *)"--data2=700"};
    int   argc   = sizeof(argv) / sizeof(char *);

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    ACA_ARGPARSE_OPT(data2, "", "data2", 0, "Test data2-value option");
    int unknownOpt = acaArgparseParse(argc, argv);
    EXPECT_EQ(unknownOpt, 0);

    // Check if error was caught
    EXPECT_EQ(data2.infoBits.hasErr, 1U);
    EXPECT_TRUE(ACA_ARGPARSE_STR_MATCH(data2.errValMsg,
                                       g_aca_argparse_err_strs[ACA_ARGPARSE_ERR_NON_VAL_OPT_VAL]));

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}
