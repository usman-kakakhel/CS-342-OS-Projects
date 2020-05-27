all: app module1 module2 module3

module1: module1.c
	gcc -c -no-pie module1.c
module2: module2.c
	gcc -c -no-pie module2.c
module3: module3.c
	gcc -c -no-pie module3.c
app: module1.o module2.o module3.o
	gcc -g -Wall -no-pie -o app module1.o module2.o module3.o
clean:
	rm -fr app *.o shm.txt
