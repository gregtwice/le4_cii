libsrc = $(wildcard lib/*.c)
objT = $(libsrc:.c=.o)
obj = $(subst lib,build,$(objT))

serversrc = $(wildcard serveur/*.c)
servObjT = $(serversrc:.c=.o)
servObj = $(subst serveur,build,$(servObjT))

all: buildDir train1.exe serveur.exe

VPATH = lib

buildDir:
	mkdir -p build

build/%.o: %.c %.h

	gcc -c $< -o $@

build/log.o: lib/log.c lib/log.h
	gcc -c $< -o $@ -DLOG_USE_COLOR

clean :
	rm build/*.o

train1.exe : main.c $(obj)
	gcc build/*.o main.c -o train1.exe

serveur.exe : serveur/ressource_manager.c
	gcc $^ -o serveur.exe -lpthread