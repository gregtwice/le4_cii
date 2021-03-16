//
// Created by gregoire on 15/03/2021.
//

#include "train_parser.h"

static void getInstruction(const char *buffer, const order_type *orderType, char *type, train_order_t *trainOrder);

static void removeTraillingWhitespace(char *buffer);


trainSequence_t *parseTrainSequence(FILE *trainfile) {
    static trainSequence_t sequence;
    sequence.nOrders = 0;
    int buffer_len = 1024;
    char type;
    char buffer[buffer_len];
    while (!feof(trainfile)) {
        fgets(buffer, buffer_len, trainfile);
        int instruction_length = strlen(buffer);
        if (instruction_length == 1) continue;
        removeTraillingWhitespace(buffer);
        if (buffer[0] == CHAR_ADDR) {
            switch (buffer[2]) {
                case 'A':
                    sscanf(buffer, "F A %d", &sequence.aiguillage_address);
                    break;
                case 'T':
                    sscanf(buffer, "F T %d", &sequence.troncon_address);
                    break;
                case 'I':
                    sscanf(buffer, "F I %d", &sequence.inversion_address);
                    break;
            }
        } else if (buffer[0] == CHAR_ID) {
            printf("\nID\n");
            sscanf(buffer, "$ %d", &sequence.train_id);
        } else if (buffer[0] != CHAR_COMMENT) {
            order_type orderType = (unsigned char) buffer[0];
            train_order_t trainOrder;

            getInstruction(buffer, &orderType, &type, &trainOrder);

            trainOrder.type = (unsigned char) type;
            if (strstr(buffer, "#")) {
                // on récupère le commentaire
                char *ptr;
                strtok(buffer, "#");
                ptr = strtok(NULL, "#");
                strcpy(trainOrder.comment, ptr);
            }
            sequence.orders[sequence.nOrders++] = trainOrder;
        }

    }
    printf("n° orders : %d\n", sequence.nOrders);
    return &sequence;
}


static void removeTraillingWhitespace(char *buffer) {
    size_t bufferSize = strlen(buffer);
    if (bufferSize > 0 && buffer[bufferSize - 1] == '\n') {
        buffer[bufferSize - 1] = '\0';
    }
}

static void getInstruction(const char *buffer, const order_type *orderType, char *type, train_order_t *trainOrder) {
    switch (*orderType) {
        case aiguillage:
            sscanf(buffer, "%c %d %c", type, &(*trainOrder).order.aiguillageOrder.code,
                   &(*trainOrder).order.aiguillageOrder.mode);
            break;
        case troncon:
            sscanf(buffer, "%c %d %d", type, &(*trainOrder).order.tronconOrder.code,
                   &(*trainOrder).order.tronconOrder.expected_cr);
            break;
        case inversion:
            sscanf(buffer, "%c %d", type, &(*trainOrder).order.inversionOrder.code);
            break;
        case listen_order:
            sscanf(buffer, "%c %d", type, &(*trainOrder).order.listenOrder.expected_cr);

            break;
        case prise_ressource:
            if (buffer[3] == '2')
                sscanf("%c %c %c %c", type, &(*trainOrder).order.priseRessourceOrder.num,
                       &(*trainOrder).order.priseRessourceOrder.ressources[0],
                       &(*trainOrder).order.priseRessourceOrder.ressources[1]);
            else if (buffer[3] == '1')
                sscanf("%c %c %c", type, &(*trainOrder).order.priseRessourceOrder.num,
                       &(*trainOrder).order.priseRessourceOrder.ressources[0]);
            break;
        case rendre_ressource:
            if (buffer[3] == '2')
                sscanf("%c %c %c %c", type, &(*trainOrder).order.rendreRessourceOrder.num,
                       &(*trainOrder).order.rendreRessourceOrder.ressources[0],
                       &(*trainOrder).order.rendreRessourceOrder.ressources[1]);
            else if (buffer[3] == '1')
                sscanf("%c %c %c", type, &(*trainOrder).order.rendreRessourceOrder.num,
                       &(*trainOrder).order.rendreRessourceOrder.ressources[0]);
            break;
        default:
            break;
    }
}


static void print_aiguillage_order_t(aiguillage_order_t order) {
    printf("Ordre de commande de l'aiguillage %d en %c\n", order.code, order.mode);
}

static void print_troncon_order_t(troncon_order_t order) {
    printf("Ordre de commande du tronçon %d jusqu'au capteur c%d\n", order.code, order.expected_cr);
}

static void print_listen_order_t(listen_order_t order) {
    printf("Ordre d'attente du franchissment du capteur %d\n", order.expected_cr);
}

static void print_inversion_order_t(inversion_order_t order) {
    printf("Ordre d'inversion du sens du train au tronçon %d\n", order.code);
}

static void print_prise_ressource_order_t(prise_ressource_order_t order) {
    if (order.num == 2)
        printf("Prise de deux ressources : %d et %d\n", order.ressources[0], order.ressources[1]);
    else
        printf("Prise d'une ressource: %d\n", order.ressources[0]);

}

static void print_rendre_ressource_order_t(rendre_ressource_order_t order) {
    if (order.num == 2)
        printf("Remise de deux ressources : %d et %d\n", order.ressources[0], order.ressources[1]);
    else
        printf("Remise d'une ressource: %d\n", order.ressources[0]);
}

void printOrder(train_order_t order) {
    switch (order.type) {
        case aiguillage:
            print_aiguillage_order_t(order.order.aiguillageOrder);
            break;
        case troncon:
            print_troncon_order_t(order.order.tronconOrder);
            break;
        case inversion:
            print_inversion_order_t(order.order.inversionOrder);
            break;
        case listen_order:
            print_listen_order_t(order.order.listenOrder);
            break;
        case prise_ressource:
            print_prise_ressource_order_t(order.order.priseRessourceOrder);
            break;
        case rendre_ressource:
            print_rendre_ressource_order_t(order.order.rendreRessourceOrder);
            break;
    }
}