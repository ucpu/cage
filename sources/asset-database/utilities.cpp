#include <cage-core/core.h>
#include <cage-core/files.h>

using namespace cage;

#include "utilities.h"

void read(file *f, uint64 &n)
{
	f->read(&n, sizeof(n));
}

void read(file *f, uint32 &n)
{
	f->read(&n, sizeof(n));
}

void read(file *f, string &s)
{
	uint32 n = 0;
	read(f, n);
	if (n > string::MaxLength)
		CAGE_THROW_ERROR(exception, "string length too big");
	char tmp[string::MaxLength];
	f->read(tmp, n);
	s = string(tmp, n);
}

void read(file *f, bool &n)
{
	f->read(&n, sizeof(n));
}

void write(file *f, const uint64 n)
{
	f->write(&n, sizeof(n));
}

void write(file *f, const uint32 n)
{
	f->write(&n, sizeof(n));
}

void write(file *f, const string &s)
{
	uint32 n = s.length();
	write(f, n);
	f->write(s.c_str(), n);
}

void write(file *f, const bool n)
{
	f->write(&n, sizeof(n));
}