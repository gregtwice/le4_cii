//
// Created by gregoire on 23/02/2021.
//

#ifndef CII_UTILS_H
#define CII_UTILS_H

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "constants.h"

#define MAX_REQUEST_LENGTH 80

typedef struct {
    unsigned int length;
    unsigned char trame[MAX_REQUEST_LENGTH];
} tramexway_t;

static void modbus_encapsulate(tramexway_t , unsigned char *);

void send_request(int socket, tramexway_t *);

void print_hex_array(tramexway_t tramexway);

void prefil_trame_3niveaux(tramexway_t *tramexway, unsigned char request_code);

void add_two_bites_variable(tramexway_t *,int index,int value);

#endif //CII_UTILS_H
