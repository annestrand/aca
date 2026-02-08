#include "aca_argparse.h"

#include <gtest/gtest.h>

static aca_argparse_opt_list g_clearOptlist = {ACA_ARGPARSE_HEAD_OPT, NULL};

TEST(argparse, test_valid_options) {
    // Mock argv and argc
    char *argv[] = {
        (char *)"<EXECUTABLE>",
        (char *)"-h",
        (char *)"--verbose",
        (char *)"-d",
        (char *)"6",
    };
    int argc = sizeof(argv) / sizeof(char *);

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    int unknownOpt = acaArgparseParse(argc, argv);
    EXPECT_EQ(unknownOpt, 0);

    // Basic validation of parsed opts
    EXPECT_EQ(help.infoBits.used, 1U);
    EXPECT_EQ(verbose.infoBits.used, 1U);
    EXPECT_EQ(data.infoBits.used, 1U);

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}

TEST(argparse, test_single_positional_arg) {
    // Mock argv and argc
    char *argv[] = {
        (char *)"<EXECUTABLE>",
        (char *)"-h",
        (char *)"my_file.txt",
        (char *)"--verbose",
        (char *)"-d",
        (char *)"6",
    };
    int argc = sizeof(argv) / sizeof(char *);

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    int unknownOpt = acaArgparseParse(argc, argv);
    EXPECT_EQ(unknownOpt, 0);

    // Grab first (and only) positional arg
    int posIndex = acaArgparseGetPositionalArg(argc, argv, 0);
    EXPECT_EQ(posIndex, 2);

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}

TEST(argparse, test_positional_args) {
    // Mock argv and argc
    char *argv[] = {
        (char *)"<EXECUTABLE>",
        (char *)"my_file1.txt",
        (char *)"-h",
        (char *)"my_file2.txt",
        (char *)"--verbose",
        (char *)"-d",
        (char *)"6",
        (char *)"my_file3.txt",
        (char *)"my_file4.txt",
        (char *)"my_file5.txt",
    };
    int argc            = sizeof(argv) / sizeof(char *);
    int posArgIndexes[] = {1, 3, 7, 8, 9};

    // Add and parse opts
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit");
    ACA_ARGPARSE_OPT(verbose, "v", "verbose", 0, "Add verbosity");
    ACA_ARGPARSE_OPT(data, "d", "data", 1, "Test data-value option");
    int unknownOpt = acaArgparseParse(argc, argv);
    EXPECT_EQ(unknownOpt, 0);

    // Grab all the positional args
    int posIndex = 0, i = 0;
    do {
        posIndex = acaArgparseGetPositionalArg(argc, argv, posIndex);
        EXPECT_EQ(posIndex, posArgIndexes[i++]);
    } while (!posIndex);

    // Cleanup HEAD node
    acaArgparseOptionListManager(&g_clearOptlist);
}
