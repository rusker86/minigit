build: main.c
	gcc -o minigit main.c -lcrypto -lz
clean:
	rm -f minigit