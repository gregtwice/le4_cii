
#include <pthread.h>
#include "lib/utils.h"
#include "lib/train_parser.h"
#include "lib/train_orders.h"
#include "lib/networking.h"
#include "lib/log.h"
#include "lib/xway.h"

#define FILE_ERR (void *) 1

shared_info sharedInfo;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

void *socketListener() {
    socketWrapper *sockAPI;

    sockAPI = sharedInfo.sockAPI;

    printf("api %d - ", sockAPI->socket);
    // boucle de service permanante;
    while (1) {
        tramexway_t *tramexway = read_xway(sockAPI->socket);
        int station = tramexway->trame[3];
        pthread_mutex_lock(sharedInfo.accessMutex);
        log_trame(*tramexway);
        train_data *data = findTrainData(station);
        log_debug("trame belongs to %s", data->trainName);
        data->lastReceived = *tramexway;
        if (tramexway->trame[0] == UNITE_WRITE_OBJECT) {
            log_debug("C'est un write var !!");
            log_trame(*tramexway);
            // l'automate envoie une donnée (PC view ou Cr ?)
            // vérifions

            // test de l'arret d'urgence
            unsigned char adresse = tramexway->trame[4];
            unsigned char valeur = tramexway->trame[6];

            if (adresse == PC_VUE_EMERGENCY_ADDR && valeur == PC_VUE_EMERGENCY) {
                // panic
                close(data->sockGEST->socket);
                data->trainConfig->run = 2;
            } else if (adresse == PC_VUE_NBTR_ADDR) {
                data->trainConfig->nbTours = (int) valeur;
            } else if (adresse == PC_VUE_RUN_ADDR) {
                if (valeur == UNITE_RUN) data->trainConfig->run = 1;
                else if (valeur == UNITE_STOP) data->trainConfig->run = 0;
            }
        }
        // ce n'est pas critique, on laisse faire le train
        data->turn = 1;
        log_debug("Allowed %s to resume", data->trainName);
        pthread_mutex_unlock(sharedInfo.accessMutex);
    }

    pthread_exit(NULL);
}

#pragma clang diagnostic pop


void *train(void *_trainName) {
    socketWrapper *sockAPI;
//    char *trainName;
    char *trainName = (char *) _trainName;
    log_info("Nom du train  : %s", trainName);
    sockAPI = sharedInfo.sockAPI;
    train_data *trainData;
    char path[255];

    for (int i = 0; i < 2; ++i) {
        if (strcmp(sharedInfo.trainData[i].trainName, trainName) == 0)
            trainData = &sharedInfo.trainData[i];
    }

    char *confDir = "./trains/";
    char *TconfigExt = ".config";
    strcpy(path, confDir);
    strcat(path, trainName);
    strcat(path, TconfigExt);
    train_config config;
    log_warn("Path : %s", path);
    parseTrainConfig(path, &config);

    trainData->bufferLength = 0;
    trainData->turn = 1;
    trainData->trainConfig = &config;
    trainData->station = config.train_station;
    trainData->trainConfig = &config;

    printf("api %d - ", sockAPI->socket);

    log_info("Connextion au gestionnaire de ressources");
    socketWrapper sockGest = initSocket(sharedInfo.trainConfig.gestionnaire_ip, sharedInfo.trainConfig.gestionnaire_port);
    trainData->sockGEST = &sockGest;
    int sockGEST = sockGest.socket;


    FILE *train1File = NULL;
    char *configExt = ".orders";
    path[0] = '\0';
    strcpy(path, confDir);
    strcat(path, trainName);
    strcat(path, configExt);

    train1File = fopen(path, "r");

    if (train1File == NULL) {
        log_error("Couldn't open file, exiting...");
        pthread_exit(FILE_ERR);
    }
    trainSequence_t trainSequence;
    parseTrainSequence(train1File, &trainSequence);
    getchar();
    do {
        if (!sharedInfo.trainConfig.loop) {
            sharedInfo.trainConfig.nbTours--;
        }
        for (int i = 0; i < trainSequence.nOrders; ++i) {
            usleep(500000);
            printOrder(trainSequence.orders[i]);
            switch (trainSequence.orders[i].type) {
                case aiguillage:
                    commander_aiguillage(sockAPI, trainSequence.aiguillage_address, trainSequence.orders[i].order.aiguillageOrder, config.train_station);
                    break;
                case troncon:
                    alimenter_troncon(sockAPI, trainSequence.troncon_address, trainSequence.orders[i].order.tronconOrder, config.train_station);
                    break;
                case inversion:
                    commander_inversion(sockAPI, trainSequence.inversion_address, trainSequence.orders[i].order.inversionOrder, config.train_station);
                    break;
                case prise_ressource:
                    prendre_ressources(sockGEST, trainSequence.train_id, trainSequence.orders[i].order.priseRessourceOrder);
                    break;
                case rendre_ressource:
                    rendre_ressources(sockGEST, trainSequence.train_id, trainSequence.orders[i].order.rendreRessourceOrder);
                    break;
            }
//            getchar();
        }
    } while (config.loop);
    close(sockGEST);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    // vérification des arguments
    if (argc < 2) {
        log_fatal("L'usage de ce programme est : %s <nom du train> <...>", argv[0]);
        exit(-1);
    }

    // parsage du fichier de config


    pthread_mutex_t mutex;
    sharedInfo.accessMutex = &mutex;
    pthread_mutex_init(sharedInfo.accessMutex, NULL);

    train_config _exe_self_config;
    parseTrainConfig(".env", &_exe_self_config);
    sharedInfo.trainConfig = _exe_self_config;
    log_info("Connextion à l'automate");
    socketWrapper sockApi = initSocket(_exe_self_config.automate_ip, _exe_self_config.automate_port);
    pthread_mutex_init(&sockApi.writeMutex, NULL);

    sharedInfo.sockAPI = &sockApi;
    pthread_t thread_socket;
    pthread_create(&thread_socket, NULL, socketListener, NULL);

    log_set_level(_exe_self_config.log_level);

    for (int i = 1; i < argc; ++i) {
        train_data data;
        sharedInfo.trainData[i - 1] = data;
        sharedInfo.trainData[i - 1].trainName = argv[i];
    }
    pthread_t thread_train1;
    pthread_t thread_train2;

    pthread_create(&thread_train1, NULL, train, argv[1]);
    pthread_create(&thread_train2, NULL, train, argv[2]);


    // initialisation de la struct partagée

    // préparation des threads et des sockets
    pthread_join(thread_socket, NULL);

    log_debug("Fermeture de la socket automate");
    close(sockApi.socket);
    log_debug("Fermeture de la socket du gestionnaire de ressources");
}

