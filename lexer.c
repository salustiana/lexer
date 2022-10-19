#include "message.h"

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#define READLEN		8
#define HISTLEN		16
#define BUFLEN		(READLEN+HISTLEN)

int look;
char buf[BUFLEN];
ssize_t rb;
size_t buf_i, hist_start;

size_t line = 1;
size_t line_char;
char *file_name = "stdin";

void init_input_buffer()
{
	rb = read(STDIN_FILENO, buf, READLEN);
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
		if (buf_i + READLEN > BUFLEN)
			rb = read(STDIN_FILENO, buf+buf_i, BUFLEN-buf_i);
		else
			rb = read(STDIN_FILENO, buf+buf_i, READLEN);

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

int get_num()
{
	int n = 0;
	int sign = 1;

	if (look == '-') {
		sign = -1;
		next_char();
	}

	if (!isdigit(look))
		expected("digit");

	if (look == '0') {
		next_char();
		return 0;
	}

	while (isdigit(look)) {
		n = n * 10 + (look - '0');
		next_char();
	}

	return n * sign;
}
