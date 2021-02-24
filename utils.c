//
// Created by gregoire on 23/02/2021.
//

#include "utils.h"

static void modbus_encapsulate(tramexway_t tramexway, unsigned char *resquest_array) {
    resquest_array[0] = 0;
    resquest_array[1] = 0;
    resquest_array[2] = 0;
    resquest_array[3] = 1;
    resquest_array[4] = 0;
    resquest_array[5] = (unsigned char) (tramexway.length + 1);
    resquest_array[6] = 0;
    for (int i = 0; i < tramexway.length; i++) {
        resquest_array[7 + i] = tramexway.trame[i];
    }
}

void send_request(int socket, tramexway_t *tramexway) {
    unsigned char requete[7 + tramexway->length];
    modbus_encapsulate(*tramexway, requete);
    write(socket, requete, sizeof(requete));
}

void print_hex_array(tramexway_t tramexway) {
    for (int i = 0; i < tramexway.length; ++i) {
        printf("%X ", tramexway.trame[i]);
    }
    printf("\n");
}

void prefil_trame(int adressage, tramexway_t *tramexway, unsigned char request_code) {
    memset(tramexway->trame,0,MAX_REQUEST_LENGTH);
    tramexway->trame[0] = adressage;
    tramexway->trame[1] = STATION_EMETEUR;
    tramexway->trame[2] = RESEAU_EMETEUR;
    tramexway->trame[3] = FIPWAY_ID; //reseau cible
    tramexway->trame[4] = AUTOMATE_TRAIN; //machine cible (train)
    tramexway->trame[5] = request_code;
    tramexway->trame[6] = 6;
    tramexway->length = 7;
}



