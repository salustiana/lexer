#include "lexer.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define ASCII_BOLD	"\033[1m"
#define ASCII_RED	"\033[31m"
#define ASCII_NORMAL	"\033[m"

/* report error */
void error(const char *fmt, ...)
{
	fprintf(stderr, ASCII_BOLD"%s:%lu:%lu: "ASCII_RED"error: "
			ASCII_NORMAL, file_name, line, line_char);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

	fprintf(stderr, "%lu | "ASCII_BOLD ASCII_RED"%c"
			ASCII_NORMAL"\n", line, look);
}

/* report error and halt */
void panic(const char *fmt, ...)
{
	fprintf(stderr, ASCII_BOLD"%s:%lu:%lu: "ASCII_RED"error: "
			ASCII_NORMAL, file_name, line, line_char);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

	fprintf(stderr, "%lu | "ASCII_BOLD ASCII_RED"%c"
			ASCII_NORMAL"\n", line, look);
	exit(1);
}

/* report what was expected and halt */
void expected(const char *fmt, ...)
{
	fprintf(stderr, ASCII_BOLD"%s:%lu:%lu: "ASCII_RED"error: "
			ASCII_NORMAL"expected ", file_name, line, line_char);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

	fprintf(stderr, "%lu | "ASCII_BOLD ASCII_RED"%c"
			ASCII_NORMAL"\n", line, look);
	exit(1);
}
