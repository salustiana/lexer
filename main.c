#include <ctype.h>
#include <stdio.h>

#include "lexer.h"
#include "message.h"

int main()
{
	init_input_buffer();

	next_char();
	while (look != EOF) {
		int n = get_num();
		printf("integer: %d\n", n);
		next_char();
	}
}
