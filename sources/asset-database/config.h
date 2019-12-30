#ifndef guard_config_h_4f28040b_f841_4fed_9c44_9de731f6627d_
#define guard_config_h_4f28040b_f841_4fed_9c44_9de731f6627d_

#include <set>

typedef std::set<string, stringComparatorFast> StringSet;

extern ConfigString configPathInput;
extern ConfigString configPathOutput;
extern ConfigString configPathIntermediate;
extern ConfigString configPathDatabase;
extern ConfigString configPathByHash;
extern ConfigString configPathByName;
extern ConfigString configPathSchemes;
extern ConfigSint32 configNotifierPort;
extern ConfigUint64 configArchiveWriteThreshold;
extern ConfigBool configFromScratch;
extern ConfigBool configListening;
extern ConfigBool configOutputArchive;
extern StringSet configIgnoreExtensions;
extern StringSet configIgnorePaths;

void configParseCmd(int argc, const char *args[]);

#endif // guard_config_h_4f28040b_f841_4fed_9c44_9de731f6627d_
