#include <stddef.h>
#include <stdint.h>

extern int look;
extern size_t line;
extern size_t line_char;
extern char *file_name;

void init_input_buffer();

int next_char();

int unread_char();

int64_t get_int();
