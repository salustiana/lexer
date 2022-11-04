#include <inttypes.h>
#include <stddef.h>

#define TKSTRLEN	(127 + 1)

extern size_t line;
extern size_t line_char;
extern const char *file_name;

enum tk_type {
	TK_INT,		TK_FLOAT,	TK_CHAR,
	TK_STR,		TK_ID,		TK_PUNCT,
	TK_OP,		TK_UN_OP,	TK_BIN_OP,
};

enum tk_op_val {
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

struct token {
	enum tk_type type;
	uint64_t num_val;
	enum tk_op_val op_val;
	char str_val[TKSTRLEN];
	size_t len;
};

void scan_input(const char *infile);
