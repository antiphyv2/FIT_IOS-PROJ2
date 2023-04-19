PROJ=proj2
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread
CC=gcc
RM=rm -f
ZIPDEST=proj2.zip

SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))

$(PROJ) : $(OBJS)

zip:
	zip $(ZIPDEST) $(PROJ).c Makefile

clean :
	$(RM) *.o $(PROJ) 


