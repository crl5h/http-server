CC=gcc
run:
	rm -f server
	gcc -o server app/server.c
	./server --directory /app/
test:
	time -p python app/conc.py
clean:
	rm -f server
