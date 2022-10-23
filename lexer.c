#include "message.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define HALFBUF		128
#define BUFLEN		(2*HALFBUF)

int look;
char buf[BUFLEN];
ssize_t rb;
size_t buf_i, hist_start;

size_t line = 1;
size_t line_char;
char *file_name = "stdin";

void init_input_buffer()
{
	rb = read(STDIN_FILENO, buf, HALFBUF);
	hist_start = rb;
}

void next_char()
{
	look = buf[buf_i++];
	buf_i %= BUFLEN;

	if (look == '\n') {
		line++;
		line_char = 0;
	}
	else
		line_char++;

	if (buf_i == hist_start) {
		if (buf_i > HALFBUF)
			rb = read(STDIN_FILENO, buf+buf_i, BUFLEN-buf_i);
		else
			rb = read(STDIN_FILENO, buf+buf_i, HALFBUF);

		if (rb < 0)
			panic("error reading from stdin\n");
		if (rb == 0) {
			look = EOF;
			return;
		}
		hist_start = (buf_i + rb) % BUFLEN;
	}
}

int unread_char()
{
	if (buf_i == hist_start)
		return 1;
	--buf_i;
	buf_i %= BUFLEN;
	return 0;
}

void match(char c)
{
	if (look == c)
		next_char();
	else
		expected("%c", c);
}

// 0|-?[1-9][0-9]*
int64_t get_int()
{
	if (look == '0') {
		next_char();
		return 0;
	}

	int64_t n = 0;
	int sign = 1;

	if (look == '-') {
		sign = -1;
		next_char();
	}

	if (!isdigit(look) || look == '0')
		expected("non-zero digit");

	while (isdigit(look)) {
		n = n * 10 + (look - '0');
		next_char();
	}

	return n * sign;
}

// -?(0|[1-9][0-9]*)\.[0-9]*
/*
char *get_float()
{
}
*/
