NAME = philosophers

COMPILER = cc

CFLAGS  = -Wall -std=c99

compile: philosophers.o
	$(COMPILER) $(FLAGS) -o $(NAME) philosophers.o -lpthread

copy.o :
	cc -c philosophers.c


clean: 
	rm philosophers.o
