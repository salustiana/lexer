#include "message.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define ASCII_BOLD	"\033[1m"
#define ASCII_RED	"\033[31m"
#define ASCII_NORMAL	"\033[m"

extern char *file_name;
extern size_t line, line_char;

/* report error and halt */
void panic(char curr_char, const char *fmt_msg, ...)
{
	fprintf(stderr, ASCII_BOLD"%s:%zu:%zu: "ASCII_RED"error: "
		ASCII_NORMAL, file_name, line, line_char);

	va_list args;
	va_start(args, fmt_msg);
	vfprintf(stderr, fmt_msg, args);
	va_end(args);

	if (curr_char)
		fprintf(stderr, "\n\t"ASCII_BOLD"%zu |"ASCII_NORMAL" %c",
			line_char, curr_char);
	fprintf(stderr, "\n");
	exit(1);
}
