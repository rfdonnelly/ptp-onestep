all: onestep-tx run

clean:
	-rm onestep-tx

onestep-tx: onestep-tx.c
	gcc -o onestep-tx onestep-tx.c

run:
	sudo ./onestep-tx
