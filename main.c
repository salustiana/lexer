#include "lexer.h"

int main(int argc, char **argv)
{
	if (argc == 1) {
		scan_input(NULL);
		return 0;
	}

	while (--argc > 0)
		scan_input(*++argv);
}
