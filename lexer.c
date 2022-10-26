#include "message.h"

#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// TODO: increase TKSTRLEN
#define TKSTRLEN	(15 + 1)
#define HALFBUF		TKSTRLEN
#define BUFLEN		(2*HALFBUF)

#define CURR_CHAR	(buf[buf_i])
#define ADVANCE_TK()	next_char(), ++tk_len

enum token {
/* LITERALS */
		TK_INT,
		TK_FLOAT,
		TK_CHAR,
		TK_STR,

/* IDENTIFIERS */
		TK_ID,

/* PUNCTUATORS */
	/* ] [ ) ( } { ; */
		TK_PUNCT,

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
		TK_ASSIGN,
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

/* KEYWORDS
 *
 *	control-flow
 *		TK_IF,
 *		TK_ELSE,
 *		TK_LOOP,
 *		TK_BRK,
 *		TK_CONT,
 *	types
 *		TK_INT_T,
 *		TK_UINT_T,
 *		TK_FLOAT_T,
 *
 * keywords will be scanned as identifiers,
 * and then categorized using a lookup table
 * (preferably with perfect hashing)
 *
 */

size_t tk_len;
enum token tk_type;
uint64_t tk_num_val;
char tk_str_val[TKSTRLEN];

signed char buf[BUFLEN];
ssize_t rb;
size_t buf_i, hist_start;

size_t line = 1;
size_t line_char;
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
	++buf_i;
	buf_i %= BUFLEN;

	if (CURR_CHAR == '\n') {
		++line;
		line_char = 1;
	}
	else
		++line_char;

	if (buf_i == hist_start) {
		if (buf_i > HALFBUF)
			rb = read(file_desc, buf+buf_i, BUFLEN-buf_i);
		else
			rb = read(file_desc, buf+buf_i, HALFBUF);

		if (rb < 0)
			panic("error reading file", NULL);
		if (rb == 0) {
			buf[buf_i] = EOF;
			++rb;
			return;
		}

		hist_start = (buf_i + rb) % BUFLEN;
	}
}

void unread_char()
{
	if (buf_i == hist_start)
		panic("cannot further unread input: "
			"no more history left in buffer", NULL);

	// TODO: unreading '\n' breaks line_char info
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
	while (CURR_CHAR == ' ' || CURR_CHAR == '\t' || CURR_CHAR == '\n')
		next_char();
}

/* scans for an identifier
 * identifier: [_A-Za-z][_A-Za-z0-9]*
 */
size_t get_id()
{
	tk_len = 0;

	if (!isalpha(CURR_CHAR) && CURR_CHAR != '_') {
		roll_back();
		return tk_len;
	}
	size_t str_i = 0;
	tk_str_val[str_i++] = CURR_CHAR;

	ADVANCE_TK();
	while (isalpha(CURR_CHAR) || isdigit(CURR_CHAR) || CURR_CHAR == '_') {
		tk_str_val[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}
	tk_str_val[str_i] = '\0';

	tk_type = TK_ID;
	return tk_len;
}

/* scans for a literal string
 * str: \"[^\"\n]*\"
 * TODO: add escaped chars to str
 * for example: \"
 */
size_t get_str()
{
	tk_len = 0;

	if (CURR_CHAR != '\"') {
		roll_back();
		return tk_len;
	}

	ADVANCE_TK();
	size_t str_i = 0;
	while (CURR_CHAR != '\"' && CURR_CHAR != '\n' && CURR_CHAR != EOF) {
		tk_str_val[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}
	tk_str_val[str_i] = '\0';

	if (CURR_CHAR != '\"') {
		roll_back();
		return tk_len;
	}
	ADVANCE_TK();

	tk_type = TK_STR;
	return tk_len;
}

/* scans for a literal char
 * char: '[^'\n]'
 */
size_t get_char()
{
	tk_len = 0;

	if (CURR_CHAR != '\'') {
		roll_back();
		return tk_len;
	}

	ADVANCE_TK();
	if (CURR_CHAR == '\'' || CURR_CHAR == '\n') {
		roll_back();
		return tk_len;
	}
	tk_num_val = CURR_CHAR;

	ADVANCE_TK();
	if (CURR_CHAR != '\'') {
		roll_back();
		return tk_len;
	}
	ADVANCE_TK();

	tk_type = TK_CHAR;
	return tk_len;
}

/* scans for a float
 * float: -?(0|[1-9][0-9]*)\.[0-9]*
 */
size_t get_float()
{
	tk_len = 0;

	// TODO: implement atof
	// instead of calling it
#include <stdlib.h>

	size_t str_i = 0;
	char flt_str[TKSTRLEN];

	if (CURR_CHAR == '-') {
		flt_str[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}

	if (CURR_CHAR == '0') {
		flt_str[str_i++] = CURR_CHAR;
		ADVANCE_TK();
		if (!isdigit(CURR_CHAR) || CURR_CHAR == '0') {
			roll_back();
			return tk_len;
		}
	}

	while (isdigit(CURR_CHAR)) {
		flt_str[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}

	if (CURR_CHAR != '.') {
		roll_back();
		return tk_len;
	}
	flt_str[str_i++] = CURR_CHAR;	/* look == '.' */
	ADVANCE_TK();

	if (!isdigit(CURR_CHAR)) {
		roll_back();
		return tk_len;
	}

	while (isdigit(CURR_CHAR)) {
		flt_str[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}
	flt_str[str_i] = '\0';

	// tk_num_val = atof(flt_str);
	double flt = atof(flt_str);
	memcpy(&tk_num_val, &flt, 8);

	tk_type = TK_FLOAT;
	return tk_len;
}

/* scans for an int
 * int:	0|-?[1-9][0-9]*
 */
size_t get_int()
{
	tk_len = 0;

	if (CURR_CHAR == '0') {
		tk_type = TK_INT;
		return tk_len;
	}

	int neg = 0;
	if (CURR_CHAR == '-') {
		neg = 1;
		ADVANCE_TK();
	}

	if (!isdigit(CURR_CHAR) || CURR_CHAR == '0') {
		roll_back();
		return tk_len;
	}

	uint64_t n = 0;
	while (isdigit(CURR_CHAR)) {
		n = n * 10 + (CURR_CHAR - '0');
		ADVANCE_TK();
	}

	if (neg)
		n = ~n + 1;
	tk_num_val = n;

	tk_type = TK_INT;
	return tk_len;
}

size_t get_token()
{
	if (get_str())
		return tk_len;
	if (get_char())
		return tk_len;
	if (get_id())
		return tk_len;
	if (get_float())
		return tk_len;
	if (get_int())
		return tk_len;

	return tk_len;
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
			panic("could not open file", NULL);
	}

	init_input_buffer();

	while (get_token()) {
		print_token();
		skip_whitespace();
	}
	if (CURR_CHAR != EOF)
		panic("syntax error", "%c", buf[buf_i]);
}
