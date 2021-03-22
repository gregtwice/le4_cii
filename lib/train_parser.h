/**
 * @file train_parser.h
 */

#ifndef CII_TRAIN_PARSER_H
#define CII_TRAIN_PARSER_H

#include <stdio.h>
#include <string.h>

#define MAX_RESSOURCES_PRISES 2
#define MAX_ORDERS 80
#define MAX_COMMENT_LENGTH 100

#define CHAR_COMMENT '#'
#define CHAR_ID '$'
#define CHAR_ADDR 'F'

#define ALLOWED_ORDERS "APRILT"

typedef struct {
    int code;
} aiguillage_order_t;

typedef struct {
    int code;
    int expected_cr;
} troncon_order_t;

typedef struct {
    int expected_cr;
} listen_order_t;

typedef struct {
    int code;
} inversion_order_t;

typedef struct {
    unsigned char num;
    unsigned char ressources[MAX_RESSOURCES_PRISES];
} prise_ressource_order_t;

typedef struct {
    unsigned char num;
    unsigned char ressources[MAX_RESSOURCES_PRISES];
} rendre_ressource_order_t;

typedef enum {
    aiguillage = 'A',
    troncon = 'T',
    inversion = 'I',
    listen_order = 'L',
    prise_ressource = 'P',
    rendre_ressource = 'R',
} order_type;

typedef struct {
    order_type type;
    union {
        prise_ressource_order_t priseRessourceOrder;
        rendre_ressource_order_t rendreRessourceOrder;
        aiguillage_order_t aiguillageOrder;
        inversion_order_t inversionOrder;
        listen_order_t listenOrder;
        troncon_order_t tronconOrder;
    } order;
    char comment[MAX_COMMENT_LENGTH];
} train_order_t;



typedef struct {
    int train_id;
    int troncon_address;
    int inversion_address;
    int aiguillage_address;
    train_order_t orders[MAX_ORDERS];
    int nOrders;
} trainSequence_t;

trainSequence_t *parseTrainSequence(FILE *trainfile);

void printOrder(train_order_t order);


#endif //CII_TRAIN_PARSER_H
