all: nrl

nrl: main.c utils.c
	gcc main.c utils.c -o ../nrl -lexpat -Wall -lm

clean:
	rm -rf ../nrl ../files ../plano.pdf ../entrada.pdf
