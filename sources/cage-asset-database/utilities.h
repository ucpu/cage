#ifndef guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_
#define guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_

void read(fileClass *f, uint64 &n);
void read(fileClass *f, uint32 &n);
void read(fileClass *f, string &s);
void read(fileClass *f, bool &n);
void write(fileClass *f, const uint64 n);
void write(fileClass *f, const uint32 n);
void write(fileClass *f, const string &s);
void write(fileClass *f, const bool n);

#endif // guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_
