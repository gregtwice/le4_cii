//
// Created by gregoire on 16/03/2021.
//

#ifndef CII_TRAIN_ORDERS_H
#define CII_TRAIN_ORDERS_H

#include "train_parser.h"
#include "xway.h"

int alimenter_troncon(int sock, int adresse, troncon_order_t tronconOrder);

int commander_aiguillage(int sock, int adresse, aiguillage_order_t aiguillageOrder);

int commander_inversion(int sock, int adresse, inversion_order_t inversionOrder);

int prendre_ressources(int sock,prise_ressource_order_t order);

int rendre_ressources(int sock,rendre_ressource_order_t order);

#endif //CII_TRAIN_ORDERS_H
