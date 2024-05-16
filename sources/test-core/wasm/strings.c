#include <stdint.h>

void *malloc(unsigned long);
void free(void *);
void *memcpy(void *, const void *, unsigned long);
int32_t printf(const char *, ...);
unsigned long strlen(const char *);

char *buffer = 0;

void clear_string(void)
{
	if (buffer)
	{
		free(buffer);
		buffer = 0;
	}
}

void generate_string(uint32_t length)
{
	clear_string();
	buffer = malloc(length + 1);
	for (int32_t i = 0; i < length; i++)
		buffer[i] = 'A' + (i % 26);
	buffer[length] = 0;
}

void set_string(char *str, uint32_t length)
{
	clear_string();
	buffer = malloc(length + 1);
	memcpy(buffer, str, length);
	buffer[length] = 0;
}

char *get_string(void)
{
	return buffer;
}

uint32_t get_length(void)
{
	return strlen(buffer);
}

void print_string(void)
{
	printf("%s", buffer);
}
