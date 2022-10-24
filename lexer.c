#include "message.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define TKSTRLEN	(256 + 1)
#define HALFBUF		TKSTRLEN
#define BUFLEN		(2*HALFBUF)

int look;

enum token {
/* LITERALS */
		TK_INT,
		TK_FLOAT,
		TK_CHAR,
		TK_STR,

/* IDENTIFIERS */
		TK_ID,

/* PUNCTUATORS */
		TK_LBRACK,
		TK_RBRACK,
		TK_LPAR,
		TK_RPAR,
		TK_LBRACE,
		TK_RBRACE,
		TK_SEMICOL,

/* KEYWORDS */
	/* control-flow */
		TK_IF,
		TK_ELSE,
		TK_LOOP,
		TK_BRK,
		TK_CONT,
	/* types */
		TK_INT_T,
		TK_UINT_T,
		TK_FLOAT_T,

/* OPERATORS */
	/* general */
		TK_ASTK,
		TK_PRCT,
		TK_AMP,
	/* aritmethic */
		TK_PLUS,
		TK_MINUS,
		TK_DIV,
	/* relational */
		TK_EQ,
		TK_NEQ,
		TK_GRT,
		TK_LESS,
		TK_GREQ,
		TK_LEQ,
	/* logical */
		TK_AND,
		TK_OR,
		TK_NOT,
	/* unary */
		TK_INC,
		TK_DEC,
	/* bitwise */
		TK_LSHFT,
		TK_RSHFT,
		TK_CMPL,
		TK_BXOR,
	/* assignment */
		TK_AS,
		TK_AINC,
		TK_ADEC,
		TK_AMUL,
		TK_ADIV,
		TK_AREM,
		TK_ALSFHT,
		TK_ARSFHT,
		TK_ABAND,
		TK_ABXOR,
};

size_t tk_len;
enum token tk_type;
uint64_t tk_num_val;
char tk_str_val[TKSTRLEN];

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
		++line;
		line_char = 0;
	}
	else
		++line_char;

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

void roll_back()
{
	while (tk_len > 0) {
		unread_char();
		--tk_len;
	}
}

void match(char c)
{
	next_char();
	if (look != c)
		expected("%c", c);
}

// 0|-?[1-9][0-9]*
size_t get_int()
{
	next_char(), tk_len = 1;

	if (look == '0')
		return tk_len;

	int neg = 0;
	if (look == '-') {
		neg = 1;
		next_char(), ++tk_len;
	}

	if (!isdigit(look) || look == '0') {
		roll_back();
		return tk_len;
	}

	uint64_t n = 0;
	while (isdigit(look)) {
		n = n * 10 + (look - '0');
		next_char(), ++tk_len;
	}
	unread_char(), --tk_len;

	if (neg)
		n = ~n + 1;
	tk_num_val = n;

	return tk_len;
}

void get_token()
{
	if (get_int())
		return;
	panic("could not resolve token");
}

void print_token()
{
	switch (tk_type) {
	case TK_STR: case TK_ID:
		printf("%zu\t%d(%s)\n", tk_len, tk_type, tk_str_val);
		break;
	case TK_INT: case TK_FLOAT: case TK_CHAR:
		printf("%zu\t%d(%"PRIu64")\n", tk_len, tk_type, tk_num_val);
		break;
	default:
		printf("%zu\t%d\n", tk_len, tk_type);
	}
}

void scan_input()
{
	init_input_buffer();

	while (look != EOF) {
		get_token();
		print_token();
		next_char();
	}
}
