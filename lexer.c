#include "message.h"

#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// TODO: increase TKSTRLEN
#define TKSTRLEN	(15 + 1)
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
size_t last_line_char;
int can_unread_nl;
int file_desc;
const char *file_name;

void print_buf()
{
	for (size_t i = 0; i < BUFLEN; i++)
		switch (buf[i]) {
		case '\0':
			putchar('@');
			break;
		case '\n':
			putchar('^');
			break;
		case EOF:
			putchar('D');
			break;
		default:
			putchar(buf[i]);
		}
	putchar('\n');
	for (size_t i = 0; i < buf_i; i++)
		putchar(' ');
	printf("^i\n");
	for (size_t i = 0; i < hist_start; i++)
		putchar(' ');
	printf("^hs\n");
}

void init_input_buffer()
{
	rb = read(file_desc, buf, HALFBUF);
	hist_start = rb;
}

void next_char()
{
	look = buf[buf_i++];
	buf_i %= BUFLEN;

	if (look == '\n') {
		++line;
		last_line_char = line_char;
		line_char = 1;
		can_unread_nl = 1;
	}
	else
		++line_char;

	if (buf_i == hist_start) {
		if (buf_i > HALFBUF)
			rb = read(file_desc, buf+buf_i, BUFLEN-buf_i);
		else
			rb = read(file_desc, buf+buf_i, HALFBUF);

		if (rb < 0)
			panic("error reading from %s", file_name);
		if (rb == 0) {
			look = EOF;
			return;
		}

		hist_start = (buf_i + rb) % BUFLEN;
	}
}

void unread_char()
{
	if (buf_i == hist_start) {
		if (look == EOF)
			return;

		panic("cannot further unread input: "
			"no more history left in buffer");
	}

	if (buf[buf_i] == '\n') {
		--line;
		// FIXME: maybe a better way to handle this.
		if (!can_unread_nl)
			panic("cannot unread two newlines without "
					"losing line_char info");
		line_char = last_line_char;
		can_unread_nl = 0;
	}
	else
		--line_char;

	--buf_i;
	buf_i %= BUFLEN;
}

void roll_back()
{
	while (tk_len > 0) {
		unread_char();
		--tk_len;
	}
}

void skip_whitespace()
{
	while (look == ' ' || look == '\t' || look == '\n')
		next_char();
	unread_char();
}

/* scans for an int
 * int:	0|-?[1-9][0-9]*
 */
size_t get_int()
{
	next_char(), tk_len = 1;

	if (look == '0') {
		tk_type = TK_INT;
		return tk_len;
	}

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

	tk_type = TK_INT;
	return tk_len;
}

size_t get_token()
{
	if (get_int())
		return tk_len;
	return 0;
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

void scan_input(const char *infile)
{
	if (!infile) {
		file_name = "stdin";
		file_desc = STDIN_FILENO;
	}
	else {
		file_name = infile;
		if ((file_desc = open(file_name, O_RDONLY)) == -1)
			panic("could not open file: %s", file_name);
	}

	init_input_buffer();

	while (look != EOF) {
		get_token();
		print_token();
		skip_whitespace();
	}
}
