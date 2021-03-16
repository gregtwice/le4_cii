all: train1.exe serveur.exe build/utils.o build/train_parser.o

train1.exe : build/utils.o build/train_parser.o constants.h tp1.c
	gcc constants.h build/utils.o build/train_parser.o tp1.c -o train1.exe

serveur.exe : utils.o constants.h ressource_manager.c
	gcc constants.h build/utils.o ressource_manager.c -o serveur.exe -lpthread

build/utils.o: utils.h utils.c
	gcc -c utils.c -o build/utils.o

build/train_parser.o : lib/train_parser.c lib/train_parser.h
	gcc -c lib/train_parser.c -o build/train_parser.o

