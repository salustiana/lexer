#include "lexer.h"

#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define HALFBUF		TKSTRLEN
#define BUFLEN		(2*HALFBUF)

#define CURR_CHAR	(buf[buf_i])
#define ADVANCE_TK()	next_char(), ++curr_tk.len

#define RET_OP_TK(OP_TYPE, OP_VAL)	\
do {					\
	curr_tk.type = (OP_TYPE);	\
	curr_tk.op_val = (OP_VAL);	\
	ADVANCE_TK();			\
	return curr_tk.len;		\
} while (0)
/* the do {} while (0) prevents semicolon pitfalls */

signed char buf[BUFLEN];
ssize_t rb;
size_t buf_i, hist_start;

size_t line = 1;
size_t line_char = 1;
size_t last_line_char;
int can_unread_line;
int file_desc;
const char *file_name;

struct token curr_tk;

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

#define ASCII_BOLD	"\033[1m"
#define ASCII_RED	"\033[31m"
#define ASCII_NORMAL	"\033[m"
#define CTXLEN		51

/* report error and halt */
void panic(const char *fmt_msg, ...)
{
	void next_char();

	fprintf(stderr, ASCII_BOLD"%s:%zu:%zu: "ASCII_RED"error: "
		ASCII_NORMAL, file_name, line, line_char);

	va_list args;
	va_start(args, fmt_msg);
	vfprintf(stderr, fmt_msg, args);
	va_end(args);

	fprintf(stderr, "\n\t"ASCII_BOLD"%zu | "ASCII_RED"%c",
		line_char, CURR_CHAR);

	char ctx[CTXLEN];
	int ctx_i = 0;
	while (CURR_CHAR != '\n' && ctx_i < CTXLEN-1) {
		next_char();
	/* next_char can be used since we no longer need line_char nor line */
		ctx[ctx_i++] = CURR_CHAR;
	}
	ctx[ctx_i] = '\0';
	fprintf(stderr, ASCII_NORMAL"%s\n", ctx);

	exit(1);
}

void init_input_buffer()
{
	rb = read(file_desc, buf, HALFBUF);
	if (rb < 0)
		panic("error reading file");
	hist_start = (size_t) rb;
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
			panic("error reading file");
		if (rb == 0) {
			buf[buf_i] = EOF;
			++rb;
			return;
		}

		hist_start = (buf_i + (size_t) rb) % BUFLEN;
	}
}

void unread_char()
{
	if (buf_i == hist_start)
		panic("cannot further unread input: "
			"no more history left in buffer");

	--buf_i;
	buf_i %= BUFLEN;

	if (CURR_CHAR == '\n') {
		if (!can_unread_line)
			panic("cannot unread newline without "
				"losing line_char info");
		--line;
		line_char = last_line_char;
		can_unread_line = 0;
	}
	else
		--line_char;
}

void roll_back()
{
	while (curr_tk.len > 0) {
		unread_char();
		--curr_tk.len;
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
	curr_tk.len = 0;

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
			RET_OP_TK(TK_OP, (enum tk_op_val) CURR_CHAR);
		case '!': case '~':
			RET_OP_TK(TK_UN_OP, (enum tk_op_val) CURR_CHAR);
		case '+': case '-': case '/':
		case '>': case '<': case '^': case '=':
			RET_OP_TK(TK_BIN_OP, (enum tk_op_val) CURR_CHAR);
		default:
			return curr_tk.len;	/* curr_tk.len == 0 */
	}
}

/*
 * scans for a punctuator
 * punctuator: \[ | \] | \( | \) | \{ | \} | , | ;
 */
size_t match_punct()
{
	curr_tk.len = 0;

	switch (CURR_CHAR) {
		case '[': case ']': case '(': case ')':
		case '{': case '}': case ',': case ';':
			curr_tk.num_val = (uint64_t) CURR_CHAR;

			ADVANCE_TK();
			curr_tk.type = TK_PUNCT;
			return curr_tk.len;

		default:
			return curr_tk.len;	/* curr_tk.len == 0 */
	}

	return curr_tk.len;
}

/*
 * scans for a comment
 * multiline: \/\*([^\*]|\*+[^\/])*\*\/
 */
size_t match_comment()
{
	curr_tk.len = 0;

	if (CURR_CHAR != '/')
		return curr_tk.len;
	ADVANCE_TK();
	if (CURR_CHAR != '*') {
		roll_back();
		return curr_tk.len;
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
			return curr_tk.len;
		}
		goto non_astk;
	}
	return curr_tk.len;
}

/*
 * scans for an identifier
 * identifier: [_A-Za-z][_A-Za-z0-9]*
 */
size_t match_id()
{
	curr_tk.len = 0;

	if (!isalpha(CURR_CHAR) && CURR_CHAR != '_')
		return curr_tk.len;
	size_t str_i = 0;
	curr_tk.str_val[str_i++] = CURR_CHAR;

	ADVANCE_TK();
	while (isalpha(CURR_CHAR) || isdigit(CURR_CHAR) ||
			CURR_CHAR == '_') {
		curr_tk.str_val[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}
	curr_tk.str_val[str_i] = '\0';

	curr_tk.type = TK_ID;
	return curr_tk.len;
}

/*
 * scans for a literal string
 * str:	\"([^\"\n\t]|\\[\"nt])*\"
 */
size_t match_str()
{
	curr_tk.len = 0;

	if (CURR_CHAR != '"')
		return curr_tk.len;

	ADVANCE_TK();
	size_t str_i = 0;
	while (CURR_CHAR != '"' && CURR_CHAR != '\n' &&
		CURR_CHAR != '\t' && CURR_CHAR != EOF) {
		if (CURR_CHAR == '\\') {
			ADVANCE_TK();
			switch (CURR_CHAR) {
				case '"':
					curr_tk.str_val[str_i++] = '"';
					break;
				case 'n':
					curr_tk.str_val[str_i++] = '\n';
					break;
				case 't':
					curr_tk.str_val[str_i++] = '\t';
					break;
				default:
					roll_back();
					return curr_tk.len;
			}
		}
		else
			curr_tk.str_val[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}
	curr_tk.str_val[str_i] = '\0';

	if (CURR_CHAR != '"') {
		roll_back();
		return curr_tk.len;
	}
	ADVANCE_TK();

	curr_tk.type = TK_STR;
	return curr_tk.len;
}

/*
 * scans for a literal char
 * char: '([^'\n\t]|\\['\\nt])'
 */
size_t match_char()
{
	curr_tk.len = 0;

	if (CURR_CHAR != '\'')
		return curr_tk.len;
	ADVANCE_TK();

	if (CURR_CHAR == '\'' || CURR_CHAR == '\n' ||
			CURR_CHAR == '\t') {
		roll_back();
		return curr_tk.len;
	}
	if (CURR_CHAR == '\\') {
		ADVANCE_TK();
		switch (CURR_CHAR) {
			case '\\':
				curr_tk.num_val = '\\';
				break;
			case '\'':
				curr_tk.num_val = '\'';
				break;
			case 'n':
				curr_tk.num_val = '\n';
				break;
			case 't':
				curr_tk.num_val = '\t';
				break;
			default:
				roll_back();
				return curr_tk.len;
		}
	}
	else
		curr_tk.num_val = (uint64_t) CURR_CHAR;
	ADVANCE_TK();

	if (CURR_CHAR != '\'') {
		roll_back();
		return curr_tk.len;
	}
	ADVANCE_TK();

	curr_tk.type = TK_CHAR;
	return curr_tk.len;
}

/*
 * scans for a float
 * float: -?(0|[1-9][0-9]*)\.[0-9]*
 */
size_t match_float()
{
	curr_tk.len = 0;

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
		return curr_tk.len;
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
		return curr_tk.len;
	}
	flt_str[str_i++] = CURR_CHAR;	/* CURR_CHAR == '.' */
	ADVANCE_TK();

	if (!isdigit(CURR_CHAR)) {
		roll_back();
		return curr_tk.len;
	}

	while (isdigit(CURR_CHAR)) {
		flt_str[str_i++] = CURR_CHAR;
		ADVANCE_TK();
	}
	flt_str[str_i] = '\0';

	// curr_tk.num_val = atof(flt_str);
	double flt = atof(flt_str);
	memcpy(&curr_tk.num_val, &flt, 8);

	curr_tk.type = TK_FLOAT;
	return curr_tk.len;
}

/*
 * scans for an int
 * int:	0|-?[1-9][0-9]*
 */
size_t match_int()
{
	curr_tk.len = 0;

	if (CURR_CHAR == '0') {
		curr_tk.type = TK_INT;
		ADVANCE_TK();
		return curr_tk.len;
	}

	int neg = 0;
	if (CURR_CHAR == '-') {
		neg = 1;
		ADVANCE_TK();
	}

	if (!isdigit(CURR_CHAR) || CURR_CHAR == '0') {
		roll_back();
		return curr_tk.len;
	}

	uint64_t n = 0;
	while (isdigit(CURR_CHAR)) {
		n = n * 10 + ((uint64_t) CURR_CHAR - '0');
		ADVANCE_TK();
	}

	if (neg)
		n = ~n + 1;
	curr_tk.num_val = n;

	curr_tk.type = TK_INT;
	return curr_tk.len;
}

size_t get_token()
{
	if (match_comment())
		skip_whitespace();
	if (match_str())
		return curr_tk.len;
	if (match_char())
		return curr_tk.len;
	if (match_id())
		return curr_tk.len;
	if (match_float())
		return curr_tk.len;
	if (match_int())
		return curr_tk.len;
	if (match_op())
		return curr_tk.len;
	if (match_punct())
		return curr_tk.len;

	return curr_tk.len;
}

void print_token(struct token tk)
{
	switch (tk.type) {
	case TK_STR: case TK_ID:
		printf("%2zu  <%d, %s>\n", tk.len, tk.type, tk.str_val);
		break;
	case TK_INT: case TK_FLOAT:
		printf("%2zu  <%d, %"PRIu64">\n", tk.len, tk.type, tk.num_val);
		break;
	case TK_CHAR: case TK_PUNCT:
		printf("%2zu  <%d, %c>\n", tk.len, tk.type, (char) tk.num_val);
		break;
	case TK_OP: case TK_UN_OP: case TK_BIN_OP:
		printf("%2zu  <%d, %d>\n", tk.len, tk.type, tk.op_val);
		break;
	default:
		printf("%2zu  <%d,>\n", tk.len, tk.type);
	}
}

void init_lexer(const char *infile)
{
	if (!infile) {
		file_name = "stdin";
		file_desc = STDIN_FILENO;
	}
	else {
		file_name = infile;
		if ((file_desc = open(file_name, O_RDONLY)) == -1)
			panic("could not open file");
	}

	init_input_buffer();
}

int next_token(struct token *tk)
{
	if (get_token()) {
		*tk = curr_tk;
		skip_whitespace();
		return 1;
	}
	if (CURR_CHAR != EOF)
		panic("syntax error");
	return 0;
}
