CC = gcc
OBJS = main.c lexer.c message.c
CFLAGS = -Wall -Wextra -Wconversion -pedantic -std=c99 -g
INCLUDES = -iquote ./includes
#LIBS = -lGL -lglfw -ldl -lm -lmi

a.out: ${OBJS}
	${CC} ${OBJS} ${INCLUDES} ${CFLAGS} #${LIBS}
