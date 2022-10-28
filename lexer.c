#include "message.h"

#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TKSTRLEN	(127 + 1)
#define HALFBUF		TKSTRLEN
#define BUFLEN		(2*HALFBUF)

#define CURR_CHAR	(buf[buf_i])
#define ADVANCE_TK()	next_char(), ++tk_len

#define RET_OP_TK(OP_TYPE, OP_VAL)	\
do {					\
	tk_type = (OP_TYPE);		\
	tk_op_val = (OP_VAL);		\
	ADVANCE_TK();			\
	return tk_len;			\
} while (0)
/* the do {} while (0) prevents semicolon pitfalls */

enum token {
	TK_INT,		TK_FLOAT,	TK_CHAR,
	TK_STR,		TK_ID,		TK_PUNCT,
	TK_OP,		TK_UN_OP,	TK_BIN_OP,
};

enum op {
	OP_ASTK = '*',	OP_PRCT = '%',	OP_AMP = '&',
	OP_PLUS = '+',	OP_MINUS = '-',	OP_DIV = '/',
	OP_GRT = '>',	OP_LESS = '<',	OP_ASSIGN = '=',
	OP_NOT = '!',	OP_BXOR = '^',	OP_CMPL = '~',
	/*
	 * it is important for '~' to be the last
	 * specified enum value, since it is the
	 * biggest ascii char. otherwise, values in
	 * the subsequent enum members would
	 * overlap with the previous ones.
	 */
	OP_EQ,		OP_NEQ,		OP_GREQ,
	OP_LEQ,		OP_AND,		OP_OR,
	OP_INC,		OP_DEC,		OP_LSHFT,
	OP_RSHFT, 	OP_AINC, 	OP_ADEC,
	OP_AMUL,	OP_ADIV,	OP_AREM,
	OP_ALSHFT,	OP_ARSHFT,	OP_ABAND,
	OP_ABXOR,
};

/*
 * KEYWORDS
 *
 *	control-flow
 *		TK_IF, TK_ELSE, TK_LOOP,
 *		TK_BRK, TK_CONT,
 *	types
 *		TK_INT_T, TK_UINT_T,
 *		TK_FLOAT_T,
 *
 * keywords will be scanned as identifiers,
 * and then categorized using a lookup table
 * (preferably with perfect hashing)
 */

size_t tk_len;
enum token tk_type;
enum op tk_op_val;
uint64_t tk_num_val;
char tk_str_val[TKSTRLEN];

signed char buf[BUFLEN];
ssize_t rb;
size_t buf_i, hist_start;

size_t line = 1;
size_t line_char = 1;
size_t last_line_char;
int can_unread_line;
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
	if (CURR_CHAR == '\n') {
		++line;
		last_line_char = line_char;
		line_char = 1;
		can_unread_line = 1;
	}
	else
		++line_char;

	++buf_i;
	buf_i %= BUFLEN;

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

	--buf_i;
	buf_i %= BUFLEN;

	if (CURR_CHAR == '\n') {
		if (!can_unread_line)
			panic("cannot unread newline without "
				"losing line_char info", NULL);
		--line;
		line_char = last_line_char;
		can_unread_line = 0;
	}
	else
		--line_char;
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
	while (CURR_CHAR == ' ' || CURR_CHAR == '\t' ||
			CURR_CHAR == '\n')
		next_char();
}

/* scans for an operator */
size_t match_op()
{
	tk_len = 0;

	/* THREE CHAR */
	if (CURR_CHAR == '>') {
		ADVANCE_TK();
		if (CURR_CHAR == '>') {
			ADVANCE_TK();
			if (CURR_CHAR == '=')
				RET_OP_TK(TK_BIN_OP, OP_ARSHFT);
			else
				roll_back();
		}
		else
			roll_back();
	}
	if (CURR_CHAR == '<') {
		ADVANCE_TK();
		if (CURR_CHAR == '<') {
			ADVANCE_TK();
			if (CURR_CHAR == '=')
				RET_OP_TK(TK_BIN_OP, OP_ALSHFT);
			else
				roll_back();
		}
		else
			roll_back();
	}

	/* TWO CHAR */
	if (CURR_CHAR == '=') {
		ADVANCE_TK();
		if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_EQ);
		else
			roll_back();
	}
	if (CURR_CHAR == '!') {
		ADVANCE_TK();
		if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_NEQ);
		else
			roll_back();
	}
	if (CURR_CHAR == '>') {
		ADVANCE_TK();
		if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_GREQ);
		else if (CURR_CHAR == '>')
			RET_OP_TK(TK_UN_OP, OP_RSHFT);
		else
			roll_back();
	}
	if (CURR_CHAR == '<') {
		ADVANCE_TK();
		if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_LEQ);
		else if (CURR_CHAR == '<')
			RET_OP_TK(TK_UN_OP, OP_LSHFT);
		else
			roll_back();
	}
	if (CURR_CHAR == '&') {
		ADVANCE_TK();
		if (CURR_CHAR == '&')
			RET_OP_TK(TK_BIN_OP, OP_AND);
		else if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_ABAND);
		else
			roll_back();
	}
	if (CURR_CHAR == '|') {
		ADVANCE_TK();
		if (CURR_CHAR == '|')
			RET_OP_TK(TK_BIN_OP, OP_OR);
		else
			roll_back();
	}
	if (CURR_CHAR == '+') {
		ADVANCE_TK();
		if (CURR_CHAR == '+')
			RET_OP_TK(TK_UN_OP, OP_INC);
		else if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_AINC);
		else
			roll_back();
	}
	if (CURR_CHAR == '-') {
		ADVANCE_TK();
		if (CURR_CHAR == '-')
			RET_OP_TK(TK_UN_OP, OP_DEC);
		else if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_ADEC);
		else
			roll_back();
	}
	if (CURR_CHAR == '*') {
		ADVANCE_TK();
		if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_AMUL);
		else
			roll_back();
	}
	if (CURR_CHAR == '/') {
		ADVANCE_TK();
		if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_ADIV);
		else
			roll_back();
	}
	if (CURR_CHAR == '%') {
		ADVANCE_TK();
		if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_AREM);
		else
			roll_back();
	}
	if (CURR_CHAR == '^') {
		ADVANCE_TK();
		if (CURR_CHAR == '=')
			RET_OP_TK(TK_BIN_OP, OP_ABXOR);
		else
			roll_back();
	}

	/* SINGLE CHAR */
	switch (CURR_CHAR) {
		case '*': case '%': case '&':
			RET_OP_TK(TK_OP, CURR_CHAR);
		case '!': case '~':
			RET_OP_TK(TK_UN_OP, CURR_CHAR);
		case '+': case '-': case '/':
		case '>': case '<': case '^': case '=':
			RET_OP_TK(TK_BIN_OP, CURR_CHAR);
		default:
			return tk_len;	/* tk_len == 0 */
	}
}

/*
 * scans for a punctuator
 * punctuator: \[ | \] | \( | \) | \{ | \} | , | ;
 */
size_t match_punct()
{
	tk_len = 0;

	switch (CURR_CHAR) {
		case '[': case ']': case '(': case ')':
		case '{': case '}': case ',': case ';':
			tk_num_val = CURR_CHAR;

			ADVANCE_TK();
			tk_type = TK_PUNCT;
			return tk_len;

		default:
			return tk_len;	/* tk_len == 0 */
	}

	return tk_len;
}

/*
 * scans for a comment
 * multiline: \/\*([^\*]|\*+[^\/])*\*\/
 */
size_t match_comment()
{
	tk_len = 0;

	if (CURR_CHAR != '/')
		return tk_len;
	ADVANCE_TK();
	if (CURR_CHAR != '*') {
		roll_back();
		return tk_len;
	}
	ADVANCE_TK();
non_astk:
	while (CURR_CHAR != '*' && CURR_CHAR != EOF)
		ADVANCE_TK();
	if (CURR_CHAR == '*') {
		ADVANCE_TK();
		while (CURR_CHAR == '*')
			ADVANCE_TK();
		if (CURR_CHAR == '/') {
			ADVANCE_TK();
			return tk_len;
		}
		goto non_astk;
	}
	return tk_len;
}

/*
 * scans for an identifier
 * identifier: [_A-Za-z][_A-Za-z0-9]*
 */
size_t match_id()
{
	tk_len = 0;

	if (!isalpha(CURR_CHAR) && CURR_CHAR != '_')
		return tk_len;
	size_t str_i = 0;
	tk_str_val[str_i++] = CURR_CHAR;

	ADVANCE_TK();
	while (isalpha(CURR_CHAR) || isdigit(CURR_CHAR) ||
			CURR_CHAR == '_') {
		tk_str_val[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}
	tk_str_val[str_i] = '\0';

	tk_type = TK_ID;
	return tk_len;
}

/*
 * scans for a literal string
 * str:	\"([^\"\n\t]|\\[\"nt])*\"
 */
size_t match_str()
{
	tk_len = 0;

	if (CURR_CHAR != '\"')
		return tk_len;

	ADVANCE_TK();
	size_t str_i = 0;
	while (CURR_CHAR != '\"' && CURR_CHAR != '\n' &&
		CURR_CHAR != '\t' && CURR_CHAR != EOF) {
		if (CURR_CHAR == '\\') {
			ADVANCE_TK();
			switch (CURR_CHAR) {
				case '"':
					tk_str_val[str_i++] = '"';
					break;
				case 'n':
					tk_str_val[str_i++] = '\n';
					break;
				case 't':
					tk_str_val[str_i++] = '\t';
					break;
				default:
					roll_back();
					return tk_len;
			}
		}
		else
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

/*
 * scans for a literal char
 * char: '([^'\n\t]|\\['\\nt])'
 */
size_t match_char()
{
	tk_len = 0;

	if (CURR_CHAR != '\'')
		return tk_len;
	ADVANCE_TK();

	if (CURR_CHAR == '\'' || CURR_CHAR == '\n' ||
			CURR_CHAR == '\t') {
		roll_back();
		return tk_len;
	}
	if (CURR_CHAR == '\\') {
		ADVANCE_TK();
		switch (CURR_CHAR) {
			case '\\':
				tk_num_val = '\\';
				break;
			case '\'':
				tk_num_val = '\'';
				break;
			case 'n':
				tk_num_val = '\n';
				break;
			case 't':
				tk_num_val = '\t';
				break;
			default:
				roll_back();
				return tk_len;
		}
	}
	else
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

/*
 * scans for a float
 * float: -?(0|[1-9][0-9]*)\.[0-9]*
 */
size_t match_float()
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

	if (!isdigit(CURR_CHAR)) {
		roll_back();
		return tk_len;
	}

	if (CURR_CHAR == '0') {
		flt_str[str_i++] = CURR_CHAR;
		ADVANCE_TK();
		goto match_fract;
	}

	while (isdigit(CURR_CHAR)) {
		flt_str[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}

match_fract:
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

/*
 * scans for an int
 * int:	0|-?[1-9][0-9]*
 */
size_t match_int()
{
	tk_len = 0;

	if (CURR_CHAR == '0') {
		tk_type = TK_INT;
		ADVANCE_TK();
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
	if (match_comment())
		skip_whitespace();
	if (match_str())
		return tk_len;
	if (match_char())
		return tk_len;
	if (match_id())
		return tk_len;
	if (match_float())
		return tk_len;
	if (match_int())
		return tk_len;
	if (match_op())
		return tk_len;
	if (match_punct())
		return tk_len;

	return tk_len;
}

void print_token()
{
	switch (tk_type) {
	case TK_STR: case TK_ID:
		printf("%2zu  <%d, %s>\n", tk_len, tk_type, tk_str_val);
		break;
	case TK_INT: case TK_FLOAT:
		printf("%2zu  <%d, %"PRIu64">\n", tk_len, tk_type, tk_num_val);
		break;
	case TK_CHAR: case TK_PUNCT:
		printf("%2zu  <%d, %c>\n", tk_len, tk_type, (char) tk_num_val);
		break;
	case TK_OP: case TK_UN_OP: case TK_BIN_OP:
		printf("%2zu  <%d, %d>\n", tk_len, tk_type, tk_op_val);
		break;
	default:
		printf("%2zu  <%d,>\n", tk_len, tk_type);
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
		panic("syntax error", "%c", CURR_CHAR);
}
