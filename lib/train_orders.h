//
// Created by gregoire on 16/03/2021.
//

#ifndef CII_TRAIN_ORDERS_H
#define CII_TRAIN_ORDERS_H

#include "train_parser.h"
#include "xway.h"

int alimenter_troncon(socketWrapper *sock, int adresse, troncon_order_t tronconOrder, int station);

int commander_aiguillage(socketWrapper *sock, int adresse, aiguillage_order_t aiguillageOrder, int station);

int commander_inversion(socketWrapper *sock, int adresse, inversion_order_t inversionOrder, int station);

int prendre_ressources(int sock, unsigned char id, prise_ressource_order_t order);

int rendre_ressources(int sock, unsigned char id, rendre_ressource_order_t order);

#endif //CII_TRAIN_ORDERS_H
