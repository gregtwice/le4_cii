//
// Created by gregoire on 16/03/2021.
//

#ifndef CII_NETWORKING_H
#define CII_NETWORKING_H




#include "utils.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int initSocket(char *addr, int port);


#endif //CII_NETWORKING_H
