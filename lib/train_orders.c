//
// Created by gregoire on 16/03/2021.
//

#include "train_orders.h"


int alimenter_troncon(int sock, int adresse, troncon_order_t tronconOrder) {
    // envoyer writeVar
    unsigned int data[1] = {tronconOrder.code};
    write_internal_word(sock, adresse, 1, data);
    // attendre le cr de l'automate
    int cr = wait_api_action(sock);
    if (cr != tronconOrder.expected_cr) {
        return -1;
    }
    return 0;
}

int commander_aiguillage(int sock, int adresse, aiguillage_order_t aiguillageOrder) {
    // envoyer writeVar
    unsigned int data[1];
    data[0] = aiguillageOrder.code;
    write_internal_word(sock, adresse, 1, data);
    // attendre le cr de l'automate
    int cr = wait_api_action(sock);
    if (cr != aiguillageOrder.code) {
        return -1;
    }
    return 0;
}

int commander_inversion(int sock, int adresse, inversion_order_t inversionOrder) {
    unsigned int data[1];
    data[0] = inversionOrder.code;
    write_internal_word(sock, adresse, 1, data);
    // attendre le cr de l'automate
    int cr = wait_api_action(sock);
    if (cr != inversionOrder.code) {
        return -1;
    }
    return 0;
}

int prendre_ressources(int sock, prise_ressource_order_t order) {

    int tailleMessage = order.num + 3;
    unsigned char REQUEST_BUFFER[tailleMessage];

    REQUEST_BUFFER[0] = 1;//id du train
    REQUEST_BUFFER[1] = (unsigned char) order.num;
    REQUEST_BUFFER[2] = 1; // prise de ressources
    for (int i = 0; i < order.num; ++i) {
        REQUEST_BUFFER[3 + i] = (int) order.ressources[i];
    }
//    for (int i = 0; i < tailleMessage; ++i) {
//        printf("[%d] = %X", i, REQUEST_BUFFER[i]);
//    }
//    printf("\n");
    write(sock, REQUEST_BUFFER, tailleMessage);
    read(sock, REQUEST_BUFFER, tailleMessage);
    return 0;
}

int rendre_ressources(int sock, rendre_ressource_order_t order) {
    int tailleMessage = order.num + 3;
    unsigned char REQUEST_BUFFER[tailleMessage];

    REQUEST_BUFFER[0] = 1;//id du train
    REQUEST_BUFFER[1] = (unsigned char) order.num;
    REQUEST_BUFFER[2] = 2; // rendu de ressources
    for (int i = 0; i < order.num; ++i) {
        REQUEST_BUFFER[3 + i] = (int) order.ressources[i];
    }
//    for (int i = 0; i < tailleMessage; ++i) {
//        printf("[%d] = %X", i, REQUEST_BUFFER[i]);
//    }
//    printf("\n");
    write(sock, REQUEST_BUFFER, tailleMessage);
    read(sock, REQUEST_BUFFER, tailleMessage);
/*    for (int i = 0; i < tailleMessage; ++i) {
        printf("%X", REQUEST_BUFFER[i]);
    }
    fflush(stdout);*/

    return 0;
}
