//
// Created by gregoire on 15/03/2021.
//

#include "train_parser.h"
#include "log.h"

static void getInstruction(const char *buffer, const order_type *orderType, char *type, train_order_t *trainOrder);

static void removeTraillingWhitespace(char *buffer);

int validerSequence(trainSequence_t sequence);


trainSequence_t *parseTrainSequence(FILE *trainfile) {
    log_info("Parsage du fichier de commande");
    static trainSequence_t sequence;
    sequence.nOrders = 0;
    int buffer_len = 1024;
    char type;
    int nT = 0, nI = 0, nA = 0, nR = 0;
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
            sscanf(buffer, "$ %d", &sequence.train_id);
        } else if (strchr(ALLOWED_ORDERS, buffer[0])) {
            order_type orderType = (unsigned char) buffer[0];
            train_order_t trainOrder;
            trainOrder.type = orderType;
            getInstruction(buffer, &orderType, &type, &trainOrder);

            if (strstr(buffer, "#")) {
                // on récupère le commentaire
                char *ptr;
                strtok(buffer, "#");
                ptr = strtok(NULL, "#");
                strcpy(trainOrder.comment, ptr);
            }
            sequence.orders[sequence.nOrders++] = trainOrder;
            switch (trainOrder.type) {
                case aiguillage:
                    nA++;
                    break;
                case troncon:
                    nT++;
                    break;
                case inversion:
                    nI++;
                    break;
                case prise_ressource:
                case rendre_ressource:
                    nR++;
                    break;
            }
        }

    }
    if (validerSequence(sequence) == -1) {
        return NULL;
    }
    log_info("Parsage terminé...");
    log_debug("Id du Train :  %d", sequence.train_id);
    log_debug("Adresse MW%d : %d Instructions Tronçons trouvées", sequence.troncon_address, nT);
    log_debug("Adresse MW%d : %d Instructions Inversion trouvées", sequence.inversion_address, nI);
    log_debug("Adresse MW%d : %d Instructions Aiguillage trouvées", sequence.aiguillage_address, nA);
    log_debug("Gestionnaire de Ressources : %d Instructions Ressources trouvées", nR);
    log_debug("Total d'instructions : %d", sequence.nOrders);
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
            sscanf(buffer, "%c %d", type, &trainOrder->order.aiguillageOrder.code);
            break;
        case troncon:
            sscanf(buffer, "%c %d %d", type, &trainOrder->order.tronconOrder.code, &(*trainOrder).order.tronconOrder.expected_cr);
            break;
        case inversion:
            sscanf(buffer, "%c %d", type, &trainOrder->order.inversionOrder.code);
            break;
        case listen_order:
            sscanf(buffer, "%c %d", type, &trainOrder->order.listenOrder.expected_cr);

            break;
        case prise_ressource:
            if (buffer[2] == '2') {
                char num;
                char r1;
                char r2;
                sscanf(buffer, "P %c %c %c", &num, &r1, &r2);
                trainOrder->order.priseRessourceOrder.num = num - 48;
                trainOrder->order.priseRessourceOrder.ressources[0] = r1 - 48;
                trainOrder->order.priseRessourceOrder.ressources[1] = r2 - 48;
            } else if (buffer[2] == '1') {
                char num;
                char r1;
                sscanf(buffer, "P %c %c", &num, &r1);
                trainOrder->order.priseRessourceOrder.num = num - 48;
                trainOrder->order.priseRessourceOrder.ressources[0] = r1 - 48;
            }
            break;
        case rendre_ressource:
            if (buffer[2] == '2') {
                char num;
                char r1;
                char r2;
                sscanf(buffer, "R %c %c %c", &num, &r1, &r2);
                trainOrder->order.rendreRessourceOrder.num = num - 48;
                trainOrder->order.rendreRessourceOrder.ressources[0] = r1 - 48;
                trainOrder->order.rendreRessourceOrder.ressources[1] = r2 - 48;
            } else if (buffer[2] == '1') {
                char num;
                char r1;
                sscanf(buffer, "R %c %c", &num, &r1);
                trainOrder->order.rendreRessourceOrder.num = num - 48;
                trainOrder->order.rendreRessourceOrder.ressources[0] = r1 - 48;
            }
            break;
        default:
            break;
    }
}

int validerSequence(trainSequence_t sequence) {
    int ressources[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < sequence.nOrders; ++i) {
        train_order_t current = sequence.orders[i];
        if (current.type == prise_ressource) {
            for (int j = 0; j < current.order.priseRessourceOrder.num; ++j) {
                ressources[current.order.priseRessourceOrder.ressources[j]] += 1;
            }
        }
        if (current.type == rendre_ressource) {
            for (int j = 0; j < current.order.rendreRessourceOrder.num; ++j) {
                ressources[current.order.rendreRessourceOrder.ressources[j]] -= 1;
            }
        }
    }
    log_info("Vérification des ressources");
    int flag = 0;
    for (int i = 0; i < 10; ++i) {
        int r = ressources[i];
        if (r != 0) {
            if (r > 0)
                log_fatal("La ressource %d n'est pas rendue", i);
            else
                log_fatal("La ressource %d est trop rendue", i);
            flag = -1;
        }
    }
    return flag;
}


static void print_aiguillage_order_t(aiguillage_order_t order) {
    log_info("commande de l'aiguillage de code : %d", order.code);
}

static void print_troncon_order_t(troncon_order_t order) {
    log_info("alimentation du tronçon %d jusqu'au capteur c%d", order.code, order.expected_cr);
}

static void print_listen_order_t(listen_order_t order) {
    log_info("Ordre d'attente du franchissment du capteur %d", order.expected_cr);
}

static void print_inversion_order_t(inversion_order_t order) {
    log_info("inversion du sens du train au tronçon %d", order.code);
}

static void print_prise_ressource_order_t(prise_ressource_order_t order) {
    if (order.num == 2)
        log_info("Prise de deux ressources : %d et %d", order.ressources[0], order.ressources[1]);
    else
        log_info("Prise d'une ressource: %d", order.ressources[0]);

}

static void print_rendre_ressource_order_t(rendre_ressource_order_t order) {
    if (order.num == 2)
        log_info("Remise de deux ressources : %d et %d", order.ressources[0], order.ressources[1]);
    else
        log_info("Remise d'une ressource: %d", order.ressources[0]);
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

