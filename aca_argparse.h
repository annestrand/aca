#ifndef ACA_ARGPARSE_H
#define ACA_ARGPARSE_H

#include <assert.h>
#include <stdio.h>
#include <string.h>

enum aca_argparse_bool { ACA_ARGPARSE_FALSE = 0, ACA_ARGPARSE_TRUE };

typedef struct aca_argparse_info_bits {
    unsigned int hasValue : 1;
    unsigned int hasErr : 1;
    unsigned int used : 1;
    unsigned int duplicate : 1;
    unsigned int longOpt : 1;
} aca_argparse_info_bits;

typedef struct aca_argparse_opt {
    const char              *shortName;
    const char              *longName;
    const char              *description;
    const char              *value;
    const char              *errValMsg;
    int                      index;
    aca_argparse_info_bits   infoBits;
    struct aca_argparse_opt *next;
} aca_argparse_opt;

typedef struct aca_argparse_opt_list {
    unsigned int      makeHeadNode;
    aca_argparse_opt *opt;
} aca_argparse_opt_list;

#define ACA_ARGPARSE_STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)
#define ACA_ARGPARSE_STR_N_MATCH(str1, str2, n) (strncmp(str1, str2, n) == 0)
#define ACA_ARGPARSE_STR_USED(val) (!((val) == NULL) && !(ACA_ARGPARSE_STR_MATCH(val, "")))
#define ACA_ARGPARSE_APPEND_OPT 0
#define ACA_ARGPARSE_HEAD_OPT 1
#define ACA_ARGPARSE_HEAD NULL
#define ACA_ARGPARSE_OPT(option, sName, lName, hasVal, desc)                                       \
    assert(!ACA_ARGPARSE_STR_MATCH("-", sName) &&                                                  \
           "[aca_argparse]: ERROR - The '-' character is not permitted as a "                      \
           "shortName");                                                                           \
    assert(!(strlen(sName) > 1) && "[aca_argparse]: ERROR - The shortName "                        \
                                   "string can only have 1 character");                            \
    aca_argparse_opt option = {"-" sName, "--" lName, desc, "", "", 0, {hasVal, 0, 0}, NULL};      \
    do {                                                                                           \
        if (strcmp((option).shortName, "-") == 0) {                                                  \
            (option).shortName = "";                                                                 \
        }                                                                                          \
        if (strcmp((option).longName, "--") == 0) {                                                  \
            (option).longName = "";                                                                  \
        }                                                                                          \
        aca_argparse_opt_list opt = {ACA_ARGPARSE_APPEND_OPT, &(option)};                            \
        acaArgparseOptionListManager(&opt);                                                        \
    } while (0)

// Global error strings
enum aca_argparse_err_indexes {
    ACA_ARGPARSE_ERR_MALFORMED_OPT_VAL,
    ACA_ARGPARSE_ERR_OPT_VAL_END_ARGV,
    ACA_ARGPARSE_ERR_VAL_IS_OPT,
    ACA_ARGPARSE_ERR_NON_VAL_OPT_VAL
};

// aca_arpgarse library main api
int               acaArgparseParse(int argc, char *argv[]);
int               acaArgparseGetPositionalArg(int argc, char *argv[], int argvOffset);
void              acaArgparsePrint();
aca_argparse_opt *acaArgparseOptionListManager(aca_argparse_opt_list *option);

#ifdef ACA_ARGPARSE_IMPLEMENTATION

const char *gAcaArgparseErrStrs[] = {"Malformed --<option>=<value>",
                                     "Option already at end of argv - expected value",
                                     "Value has option syntax (i.e. -, --)",
                                     "Value given on a non-value opt"};

aca_argparse_opt *acaArgparseOptionListManager(aca_argparse_opt_list *option) {
    static aca_argparse_opt *pAcaArgparseHead = NULL;
    if (option == NULL) {
        return pAcaArgparseHead;
    } else {
        if (pAcaArgparseHead == NULL || option->makeHeadNode == ACA_ARGPARSE_HEAD_OPT) {
            pAcaArgparseHead = option->opt;
            return NULL;
        }
        aca_argparse_opt *pTmp = pAcaArgparseHead;
        while (pTmp->next != NULL) {
            pTmp = pTmp->next;
        }
        pTmp->next = option->opt;
        return NULL;
    }
}

// Parse argv for opts and return argv index on first-found unknown option
// (returns 0 if no unknown opts)
int acaArgparseParse(int argc, char *argv[]) {
    int firstUnknownOpt = 0;
    for (int i = 1; i < argc; ++i) {
        // Check if arg is not a option-type
        if (argv[i][0] != '-') {
            continue;
        }

        int               isLongOpt = 0;
        int               validOpt  = 0;
        aca_argparse_opt *pTmp       = acaArgparseOptionListManager(ACA_ARGPARSE_HEAD);

        // Look for opt in opts list
        while (pTmp != NULL) {
            if (ACA_ARGPARSE_STR_USED(pTmp->shortName) &&
                ACA_ARGPARSE_STR_MATCH(pTmp->shortName, argv[i])) {
                if (pTmp->infoBits.used) {
                    pTmp->infoBits.duplicate = 1;
                }
                pTmp->infoBits.used = 1;
                pTmp->index         = i;
                validOpt           = 1;
            } else if (ACA_ARGPARSE_STR_USED(pTmp->longName) && !pTmp->infoBits.hasValue &&
                       ACA_ARGPARSE_STR_MATCH(argv[i], pTmp->longName)) {
                if (pTmp->infoBits.used) {
                    pTmp->infoBits.duplicate = 1;
                }
                isLongOpt          = 1;
                pTmp->infoBits.used = 1;
                pTmp->index         = i;
                validOpt           = 1;
            } else {
                char  *pVal    = strchr(argv[i], '=');
                size_t offset = (int)(pVal - argv[i]);
                if (ACA_ARGPARSE_STR_USED(pTmp->longName) &&
                    ACA_ARGPARSE_STR_N_MATCH(argv[i], pTmp->longName, offset)) {
                    if (!pTmp->infoBits.hasValue) {
                        pTmp->infoBits.hasErr = 1;
                        pTmp->errValMsg = gAcaArgparseErrStrs[ACA_ARGPARSE_ERR_NON_VAL_OPT_VAL];
                        pTmp->value     = argv[i];
                    }
                    if (pTmp->infoBits.used) {
                        pTmp->infoBits.duplicate = 1;
                    }
                    isLongOpt          = 1;
                    pTmp->infoBits.used = 1;
                    pTmp->index         = i;
                    validOpt           = 1;
                }
            }
            if (pTmp->infoBits.used && pTmp->infoBits.hasValue && validOpt) {
                if (isLongOpt) {
                    char  *pVal = nullptr;
                    size_t offset = 0;
                    pVal = strchr(argv[i], '=');
                    if (pVal == NULL) {
                        pTmp->infoBits.hasErr = 1;
                        pTmp->errValMsg = gAcaArgparseErrStrs[ACA_ARGPARSE_ERR_MALFORMED_OPT_VAL];
                        pTmp->value     = argv[i];
                    } else {
                        offset     = (int)(pVal - argv[i]);
                        pTmp->value = &argv[i][offset + 1];
                    }
                    pTmp->infoBits.longOpt = 1;
                } else {
                    if ((i + 1) >= argc) {
                        pTmp->infoBits.hasErr = 1;
                        pTmp->errValMsg = gAcaArgparseErrStrs[ACA_ARGPARSE_ERR_OPT_VAL_END_ARGV];
                    } else if (argv[i + 1][0] == '-') {
                        pTmp->infoBits.hasErr = 1;
                        pTmp->errValMsg       = gAcaArgparseErrStrs[ACA_ARGPARSE_ERR_VAL_IS_OPT];
                        pTmp->value           = argv[i + 1];
                    } else {
                        pTmp->value = argv[i + 1];
                        ++i;
                    }
                }
            }
            if (pTmp->infoBits.used && validOpt) {
                break;
            }
            pTmp = pTmp->next;
        }

        // Track first occurance of unknown opt
        if (!validOpt && firstUnknownOpt == 0) {
            firstUnknownOpt = i;
        }
    }
    return firstUnknownOpt;
}

int acaArgparseGetPositionalArg(int argc, char *argv[], int argvOffset) {
    int i = 0;
    for (i = argvOffset + 1; i < argc; ++i) {
        // Skip if opt-type value
        if (argv[i][0] == '-') {
            continue;
        }

        // Otherwise check if arg is opt-value type or not
        int               isOptValue = 0;
        aca_argparse_opt *pTmp        = acaArgparseOptionListManager(ACA_ARGPARSE_HEAD);
        while (pTmp != NULL) {
            if (pTmp->infoBits.used && pTmp->infoBits.hasValue &&
                (pTmp->index == i - 1 && !pTmp->infoBits.longOpt)) {
                isOptValue = 1;
            }
            pTmp = pTmp->next;
        }

        // Found positional arg
        if (!isOptValue) {
            return i;
        }
    }
    return 0;
}

void acaArgparsePrint() {
    aca_argparse_opt *pTmp = acaArgparseOptionListManager(ACA_ARGPARSE_HEAD);
    while (pTmp != NULL) {
        if (pTmp->infoBits.hasValue) {
            if (ACA_ARGPARSE_STR_USED(pTmp->shortName) && ACA_ARGPARSE_STR_USED(pTmp->longName)) {
                printf("  %s <value>, %s=<value>\n", pTmp->shortName, pTmp->longName);
            } else if (ACA_ARGPARSE_STR_USED(pTmp->shortName)) {
                printf("  %s <value>\n", pTmp->shortName);
            } else if (ACA_ARGPARSE_STR_USED(pTmp->longName)) {
                printf("  %s <value>\n", pTmp->longName);
            } else {
                pTmp = pTmp->next;
                continue;
            }

            if (ACA_ARGPARSE_STR_USED(pTmp->description)) {
                printf("        %s\n\n", pTmp->description);
            }
        } else {
            if (ACA_ARGPARSE_STR_USED(pTmp->shortName) && ACA_ARGPARSE_STR_USED(pTmp->longName)) {
                printf("  %s, %s\n", pTmp->shortName, pTmp->longName);
            } else if (ACA_ARGPARSE_STR_USED(pTmp->shortName)) {
                printf("  %s\n", pTmp->shortName);
            } else if (ACA_ARGPARSE_STR_USED(pTmp->longName)) {
                printf("  %s\n", pTmp->longName);
            } else {
                pTmp = pTmp->next;
                continue;
            }

            if (ACA_ARGPARSE_STR_USED(pTmp->description)) {
                printf("        %s\n\n", pTmp->description);
            }
        }
        pTmp = pTmp->next;
    }
}

#endif // ACA_ARGPARSE_IMPLEMENTATION

#endif // ACA_ARGPARSE_H
