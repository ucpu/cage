#ifndef guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_
#define guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_

void read(file *f, uint64 &n);
void read(file *f, uint32 &n);
void read(file *f, string &s);
void read(file *f, bool &n);
void write(file *f, const uint64 n);
void write(file *f, const uint32 n);
void write(file *f, const string &s);
void write(file *f, const bool n);

#endif // guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_
