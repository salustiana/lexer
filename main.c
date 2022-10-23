#include <stdio.h>

#include "lexer.h"
#include "message.h"

int main()
{
	init_input_buffer();

	next_char();
	while (look != EOF) {
		int64_t n = get_int();
		printf("integer: %ld\n", n);
		next_char();
	}
}
