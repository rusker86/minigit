build: main.c
	make clean
	gcc -o minigit main.c -lcrypto -lz
clean:
	rm -f minigit