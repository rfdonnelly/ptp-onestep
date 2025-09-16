all: run

clean:
	-rm onestep-tx

onestep-tx: onestep-tx.c
	gcc -o onestep-tx onestep-tx.c

onestep-tx-arm: onestep-tx.c
	arm-linux-gnueabihf-gcc -O3 -Wall -static -o onestep-tx-arm onestep-tx.c

run: onestep-tx
	sudo ./onestep-tx

run-arm: onestep-tx-arm
	./run hpscrpitsn0 hpscrpitsn1
