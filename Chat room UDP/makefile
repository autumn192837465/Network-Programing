all: receiver1 receiver2 receiver3 sender1 sender2 sender3 clear
receiver1: receiver1.o
	gcc receiver1.o -o receiver1
receiver1.o: receiver1.c
	gcc -c receiver1.c
receiver2: receiver2.o
	gcc receiver2.o -o receiver2
receiver2.o: receiver2.c
	gcc -c receiver2.c
receiver3: receiver3.o
	gcc receiver3.o -o receiver3
receiver3.o: receiver3.c
	gcc -c receiver3.c
sender1: sender1.o
	gcc sender1.o -o sender1
sender1.o:
	gcc -c sender1.c
sender2: sender2.o
	gcc sender2.o -o sender2
sender2.o:
	gcc -c sender2.c
sender3: sender3.o
	gcc sender3.o -o sender3
sender3.o:
	gcc -c sender3.c
clear:
	rm -f receiver1.o receiver2.o receiver3.o sender1.o sender2.o sender3.o
clean:
	rm -f receiver1 receiver2 receiver3 sender1 sender2 sender3
