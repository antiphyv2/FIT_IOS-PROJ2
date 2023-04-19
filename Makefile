PROJ=proj2
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread
CC=gcc
RM=rm -f

SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))

$(PROJ) : $(OBJS)

clean :
	$(RM) *.o $(PROJ) 


