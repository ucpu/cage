#ifndef guard_notifier_h_f1df1092_6c8f_415f_bd13_e2dd5d633b2c_
#define guard_notifier_h_f1df1092_6c8f_415f_bd13_e2dd5d633b2c_

void notifierInitialize(const uint16 port);
void notifierAccept();
void notifierNotify(const string &str);

#endif // guard_notifier_h_f1df1092_6c8f_415f_bd13_e2dd5d633b2c_
