#ifndef guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_
#define guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_

void read(File *f, uint64 &n);
void read(File *f, uint32 &n);
void read(File *f, string &s);
void read(File *f, bool &n);
void write(File *f, const uint64 n);
void write(File *f, const uint32 n);
void write(File *f, const string &s);
void write(File *f, const bool n);

#endif // guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_
