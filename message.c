#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define ASCII_BOLD	"\033[1m"
#define ASCII_RED	"\033[31m"
#define ASCII_NORMAL	"\033[m"

extern char *file_name;
extern size_t line, line_char;

/* report error and halt */
void panic(const char *msg, const char *fmt_ctx, ...)
{
	fprintf(stderr, ASCII_BOLD"%s:%zu:%zu: "ASCII_RED"error: "
		ASCII_NORMAL"%s\n\t%zu | ",
		file_name, line, line_char, msg, line_char);

	va_list args;
	va_start(args, fmt_ctx);
	vfprintf(stderr, fmt_ctx, args);
	va_end(args);

	fprintf(stderr, "\n");
	exit(1);
}
