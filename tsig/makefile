NAME = tsig

COMPILER = cc

CFLAGS  = -Wall -std=c99

compile: tsig.o
	$(COMPILER) $(FLAGS) -o $(NAME) tsig.o

tsig.o :
	cc -c tsig.c

clean: 
	rm tsig.o
