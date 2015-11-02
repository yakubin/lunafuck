CC=clang
CFLAGS=-O3 -Wall -Werror -Wextra -std=c99
SRC=addrstack.c codelist.c hex.c lunafuck.c

lunafuck: $(SRC)
	$(CC) $(CFLAGS) -o lunafuck $(SRC)

test: unittests
	./unittests
	
unittests: codelist.c unittests.c
	$(CC) $(CFLAGS) -o unittests codelist.c unittests.c

unittests.c: gentests.pl
	./gentests.pl

clean:
	rm -f lunafuck unittests unittests.c
