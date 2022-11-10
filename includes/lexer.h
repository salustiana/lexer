#include <inttypes.h>
#include <stddef.h>

#define TKSTRLEN	(127 + 1)

extern size_t line;
extern size_t line_char;
extern const char *file_name;

enum tk_type {
	/* punctuators */
	TK_SMCOL = ';',	TK_COMMA = ',',	TK_LBRCK = '[',
	TK_RBRCK = ']',	TK_LPAR = '(',	TK_RPAR = ')',
	TK_LBRCE = '{',	TK_RBRCE = '}',

	/* operators */
	TK_ASTK = '*',	TK_PRCT = '%',	TK_AMP = '&',
	TK_PLUS = '+',	TK_MINUS = '-',	TK_DIV = '/',
	TK_GRT = '>',	TK_LESS = '<',	TK_ASSIGN = '=',
	TK_NOT = '!',	TK_BXOR = '^',	TK_CMPL = '~',
	/*
	 * it is important for '~' to be the last
	 * specified enum value, since it is the
	 * biggest ascii char. otherwise, values in
	 * the subsequent enum members would
	 * overlap with the previous ones.
	 */
	TK_EQ,		TK_NEQ,		TK_GREQ,
	TK_LEQ,		TK_AND,		TK_OR,
	TK_INC,		TK_DEC,		TK_LSHFT,
	TK_RSHFT, 	TK_AINC, 	TK_ADEC,
	TK_AMUL,	TK_ADIV,	TK_AREM,
	TK_ALSHFT,	TK_ARSHFT,	TK_ABAND,
	TK_ABXOR,

	/* literals */
	TK_INT,		TK_FLOAT,	TK_CHAR,
	TK_STR,		TK_ID,

	/* keywords */
	TK_IF,		TK_ELSE,	TK_LOOP,
	TK_BRK,		TK_CONT,
	/*
	 * keywords will be scanned as identifiers,
	 * and then categorized using a lookup table
	 * (preferably with perfect hashing)
	 */

	/* types */
	TK_INT_T,	TK_UINT_T,	TK_FLOAT_T,
};

struct token {
	enum tk_type type;
	uint64_t num_val;
	char str_val[TKSTRLEN];
	size_t len;
};

/* report error and halt */
void panic(const char *fmt_msg, ...);

void init_lexer(const char *infile);

int next_token(struct token *tk);

void print_token(struct token tk);
