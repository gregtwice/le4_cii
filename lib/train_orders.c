//
// Created by gregoire on 16/03/2021.
//

#include "train_orders.h"


int alimenter_troncon(socketWrapper * sock, int adresse, troncon_order_t tronconOrder, int station) {
    // envoyer writeVar

    pthread_mutex_lock(&sock->writeMutex);
    unsigned int data[1] = {tronconOrder.code};
    write_internal_word(sock->socket, adresse, 1, data, station);
    usleep(300);

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
    unsigned int data[1] = {aiguillageOrder.code};
    pthread_mutex_lock(&sock->writeMutex);
    write_internal_word(sock->socket, adresse, 1, data, station);
    usleep(300);

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
    unsigned int data[1] = {inversionOrder.code};
    usleep(1000 * 1000);
    log_debug("MUTEX");
    pthread_mutex_lock(&sock->writeMutex);
    write_internal_word(sock->socket, adresse, 1, data, station);
    log_debug("WRITE");
    usleep(1000);
    pthread_mutex_unlock(&sock->writeMutex);
    log_debug("MUTEX_FIN");
    // attendre le cr de l'automate
    int cr = wait_api_action(sock, station);
    if (cr != inversionOrder.code) {
        return -1;
    }
    return 0;
}

int prendre_ressources(int sock, unsigned char id, prise_ressource_order_t order) {
    int tailleMessage = order.num + 3;
    unsigned char REQUEST_BUFFER[tailleMessage];
    REQUEST_BUFFER[0] = id; //id du train
    REQUEST_BUFFER[1] = (unsigned char) order.num;
    REQUEST_BUFFER[2] = 1; // prise de ressources
    for (int i = 0; i < order.num; ++i) {
        REQUEST_BUFFER[3 + i] = (int) order.ressources[i];
    }

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

    write(sock, REQUEST_BUFFER, tailleMessage);
    usleep(10000);
    read(sock, REQUEST_BUFFER, tailleMessage);

    return 0;
}
