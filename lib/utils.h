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
#include <stdlib.h>

#include "log.h"


#define MAX_REQUEST_LENGTH 80
#define MAX 80


typedef struct {
    unsigned int length;
    unsigned char trame[MAX_REQUEST_LENGTH];
} tramexway_t;

typedef struct {
    int train_station;
    int train_reseau;
    int train_porte;
    char automate_ip[15];
    int automate_port;
    int automate_station;
    int automate_reseau;
    int automate_porte;
    char gestionnaire_ip[15];
    int gestionnaire_port;
    int log_level;
    int loop;
} train_config;

extern train_config config;

static void modbus_encapsulate(tramexway_t, unsigned char *);

void send_request(int socket, tramexway_t *);

void print_hex_array(tramexway_t tramexway);

void private_log_trame(tramexway_t tramexway, char *file, int line);

void prefil_trame_3niveaux(tramexway_t *tramexway, unsigned char request_code);

void prefil_trame_5niveaux(tramexway_t *tramexway, const unsigned char *API_REQUEST);

void add_two_bites_variable(tramexway_t *, int index, int value);

train_config * parseTrainConfig(char * filename);

#define log_trame(trame) private_log_trame(trame,__FILE__,__LINE__)


#endif //CII_UTILS_H
