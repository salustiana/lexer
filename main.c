#include "lexer.h"

struct token tk;

int main(int argc, char **argv)
{
	if (argc == 1) {
		init_lexer(NULL);
		return 0;
	}

	while (--argc > 0) {
		init_lexer(*++argv);
		while (next_token(&tk))
			print_token(tk);
	}
}
