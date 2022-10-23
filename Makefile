CC = gcc
OBJS = main.c lexer.c message.c
CFLAGS = -Wall -Wextra -pedantic -std=c99
INCLUDES = -I ./includes
#LIBS = -lGL -lglfw -ldl -lm -lmi

a.out: ${OBJS}
	${CC} ${OBJS} ${INCLUDES} ${CFLAGS} #${LIBS}
