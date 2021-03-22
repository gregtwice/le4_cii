
#include <assert.h>
#include "lib/utils.h"
#include "lib/train_parser.h"
#include "lib/train_orders.h"
#include "lib/networking.h"
#include "lib/log.h"

void train(int sockAPI, int sockGEST,char * trainName) {
    FILE *train1File = NULL;
    char *confDir = "./trains/";
    char *configExt = ".orders";
    char path[255];
    strcpy(path, confDir);
    strcat(path, trainName);
    strcat(path, configExt);

    train1File = fopen(path, "r");

    if (train1File == NULL) {
        log_error("Couldn't open file, exiting...");
        return;
    }

    trainSequence_t *trainSequence = parseTrainSequence(train1File);
    assert(trainSequence != NULL);
    do {
        for (int i = 0; i < trainSequence->nOrders; ++i) {
            usleep(100000);
            printOrder(trainSequence->orders[i]);
            switch (trainSequence->orders[i].type) {
                case aiguillage:
                    commander_aiguillage(sockAPI, trainSequence->aiguillage_address, trainSequence->orders[i].order.aiguillageOrder);
                    break;
                case troncon:
                    alimenter_troncon(sockAPI, trainSequence->troncon_address, trainSequence->orders[i].order.tronconOrder);
                    break;
                case inversion:
                    commander_inversion(sockAPI, trainSequence->inversion_address, trainSequence->orders[i].order.inversionOrder);
                    break;
                case listen_order: // Verifier utilité
                    break;
                case prise_ressource:
                    prendre_ressources(sockGEST, trainSequence->orders[i].order.priseRessourceOrder);
                    break;
                case rendre_ressource:
                    rendre_ressources(sockGEST, trainSequence->orders[i].order.rendreRessourceOrder);
                    break;
            }
        }
    } while (config.loop);

}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        log_fatal("L'usage de ce programme est : %s <nom du train>", argv[0]);
        exit(-1);
    }
    char *confDir = "./trains/";
    char *configExt = ".config";
    char path[255];
    strcpy(path, confDir);
    strcat(path, argv[1]);
    strcat(path, configExt);
    config = *parseTrainConfig(path);
    log_set_level(config.log_level);



    log_info("Connextion à l'automate");
    int sockfd = initSocket(config.automate_ip, config.automate_port);

    log_info("Connextion au gestionnaire de ressources");
    int sockGest = initSocket(config.gestionnaire_ip, config.gestionnaire_port);
    train(sockfd, sockGest,argv[1]);
    log_debug("Fermeture de la socket automate");
    close(sockfd);
    log_debug("Fermeture de la socket du gestionnaire de ressources");
    close(sockGest);
}

