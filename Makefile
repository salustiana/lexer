CC = gcc
OBJS = main.c lexer.c
CFLAGS = -Wall -Wextra -Wconversion -pedantic -std=c99 -g
INCLUDES = -iquote ./include
#LIBS = -lGL -lglfw -ldl -lm -lmi

a.out: ${OBJS}
	${CC} ${OBJS} ${INCLUDES} ${CFLAGS} #${LIBS}
