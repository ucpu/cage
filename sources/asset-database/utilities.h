#ifndef guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_
#define guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_

void read(fileHandle *f, uint64 &n);
void read(fileHandle *f, uint32 &n);
void read(fileHandle *f, string &s);
void read(fileHandle *f, bool &n);
void write(fileHandle *f, const uint64 n);
void write(fileHandle *f, const uint32 n);
void write(fileHandle *f, const string &s);
void write(fileHandle *f, const bool n);

#endif // guard_uilities_h_c427e57f_913d_4798_b3c4_1423aacd6746_
