//
// Created by gregoire on 16/03/2021.
//

#include "train_orders.h"


int alimenter_troncon(socketWrapper * sock, int adresse, troncon_order_t tronconOrder, int station) {
    // envoyer writeVar
    pthread_mutex_lock(&sock->writeMutex);
    unsigned int data[1] = {tronconOrder.code};
    write_internal_word(sock->socket, adresse, 1, data, station);
    pthread_mutex_unlock(&sock->writeMutex);
    // attendre le cr de l'automate
    usleep(300);
    int cr = wait_api_action(sock, station);
    if (cr != tronconOrder.expected_cr) {
        log_warn("expected %d, got %d", tronconOrder.expected_cr, cr);
        return -1;
    }
    return 0;
}

int commander_aiguillage(socketWrapper * sock, int adresse, aiguillage_order_t aiguillageOrder, int station) {
    // envoyer writeVar
    unsigned int data[1];
    data[0] = aiguillageOrder.code;
    pthread_mutex_lock(&sock->writeMutex);
    write_internal_word(sock->socket, adresse, 1, data, station);
    pthread_mutex_unlock(&sock->writeMutex);

    // attendre le cr de l'automate
    usleep(300);

    int cr = wait_api_action(sock, station);
    if (cr != aiguillageOrder.code) {
        return -1;
    }
    return 0;
}

int commander_inversion(socketWrapper * sock, int adresse, inversion_order_t inversionOrder, int station) {
    unsigned int data[1];
    data[0] = inversionOrder.code;
    usleep(1000 * 1000);
    pthread_mutex_lock(&sock->writeMutex);
    write_internal_word(sock->socket, adresse, 1, data, station);
    pthread_mutex_unlock(&sock->writeMutex);
    usleep(1000);
    // attendre le cr de l'automate
    int cr = wait_api_action(sock, station);
    if (cr != inversionOrder.code) {
        return -1;
    }
    return 0;
}

int prendre_ressources(int sock, unsigned char id, prise_ressource_order_t order) {
    log_debug("sock : %d",sock);
    int tailleMessage = order.num + 3;
    unsigned char REQUEST_BUFFER[tailleMessage];

    REQUEST_BUFFER[0] = id;//id du train
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
    usleep(10000);
    read(sock, REQUEST_BUFFER, tailleMessage);
    return 0;
}

int rendre_ressources(int sock, unsigned char id, rendre_ressource_order_t order) {
    int tailleMessage = order.num + 3;
    unsigned char REQUEST_BUFFER[tailleMessage];

    REQUEST_BUFFER[0] = id;//id du train
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
    usleep(10000);
    read(sock, REQUEST_BUFFER, tailleMessage);
/*    for (int i = 0; i < tailleMessage; ++i) {
        printf("%X", REQUEST_BUFFER[i]);
    }
    fflush(stdout);*/

    return 0;
}
