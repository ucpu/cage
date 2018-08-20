#ifndef guard_config_h_4f28040b_f841_4fed_9c44_9de731f6627d_
#define guard_config_h_4f28040b_f841_4fed_9c44_9de731f6627d_

typedef std::set<string, stringComparatorFast> stringSet;

extern configString configPathInput;
extern configString configPathOutput;
extern configString configPathDatabase;
extern configString configPathReverse;
extern configString configPathForward;
extern configString configPathScheme;
extern configSint32 configNotifierPort;
extern configBool configFromScratch;
extern configBool configListening;
extern stringSet configIgnoreExtensions;
extern stringSet configIgnorePaths;

void configParseCmd(int argc, const char *args[]);

#endif // guard_config_h_4f28040b_f841_4fed_9c44_9de731f6627d_
