PROJ=proj2
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread
CC=gcc
RM=rm -f
ZIPDEST=proj2.zip

proj2: proj2.c
	$(CC) proj2.c $(CFLAGS) -o proj2

zip:
	zip $(ZIPDEST) $(PROJ).c Makefile

clean :
	$(RM) *.o $(PROJ) 


