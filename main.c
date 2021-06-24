
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
    // boucle de service permanente;
    while (1) {
        log_trace("Listening...");
        tramexway_t *tramexway = read_xway(sockAPI->socket);
        int station = tramexway->trame[3];
        log_trame(*tramexway);
        pthread_mutex_lock(sharedInfo.accessMutex);
        train_data *data = findTrainData(station);
        log_debug("trame belongs to %s", data->trainName);
        if (tramexway->trame[5] == 0x66) {
            if (sharedInfo.polling_run == 1) {
                for (int i = 51; i <= 54; ++i) {
                    train_data *d = findTrainData(i);
                    if (d != NULL) {
                        unsigned char octet = tramexway->trame[7];

                        switch (d->station) {
                            case 51:
                                d->trainConfig->run = (char) (octet & (0x10));
                                break;
                            case 52:
                                d->trainConfig->run = (char) (octet & (0x20));
                                break;
                            case 53:
                                d->trainConfig->run = (char) (octet & (0x40));
                                break;
                            case 54:
                                d->trainConfig->run = (char) (octet & (0x80));
                                break;
                        }
                    }
                }
                sharedInfo.polling_run = 0;
            } else if (sharedInfo.polling_tours == 1) {
                for (int i = 51; i <= 54; ++i) {
                    int currentIndex = (i - 51) * 2;
                    train_data *d = findTrainData(station);
                    if (d != NULL) {
                        unsigned int val = tramexway->trame[7 + currentIndex] + (tramexway->trame[7 + currentIndex + 1] << 8);
                        d->trainConfig->nbToursCommande = (int) val;
                    }
                }
                sharedInfo.polling_tours = 0;
            }
            pthread_mutex_unlock(sharedInfo.accessMutex);
            continue;
        }
        data->lastReceived = *tramexway;
        // ce n'est pas critique, on laisse faire le train
        data->turn = 1;
        log_debug("Allowed %s to resume", data->trainName);
        pthread_mutex_unlock(sharedInfo.accessMutex);
    }

    pthread_exit(NULL);
}

#pragma clang diagnostic pop


_Noreturn void *train(void *_trainName) {
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
    config.run = 0;
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

    do {
        if (!config.run)
            trainData->trainConfig->nbTours = trainData->trainConfig->nbToursCommande;
        while (config.run == 0) {
            usleep(10000);
        }
        for (int i = 0; i < trainSequence.nOrders; ++i) {
            if (trainData->trainConfig->run == 2) {
                pthread_exit(NULL);
            }
            usleep(50000);
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
        }
        if (trainData->trainConfig->nbTours > 0) {
            trainData->trainConfig->nbTours--;
            pthread_mutex_lock(&sockAPI->writeMutex);
            unsigned int nbTrToSend[1] = {trainData->trainConfig->nbTours};
            write_internal_word(sockAPI->socket, trainData->trainConfig->addr_tours, 1, nbTrToSend, config.train_station);
            pthread_mutex_unlock(&sockAPI->writeMutex);
        } else {
            config.run = 0;
        }
    } while (1);
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


    while (1) {
        usleep(1000 * 1000);
        for (int i = 1; i < argc; ++i) {
            if (sharedInfo.trainData[i - 1].turn == 2) {
                usleep(300);
            }
        }
        log_debug("Polling Run");
        sharedInfo.polling_run = 1;
        pthread_mutex_lock(&sockApi.writeMutex);
        pollRun(sockApi.socket, sharedInfo.trainData[0].station);
        pthread_mutex_unlock(&sockApi.writeMutex);
        while (sharedInfo.polling_run) usleep(2000);
        log_debug("Polling Tours");
        usleep(1000 * 1000);
        sharedInfo.polling_tours = 1;
        pthread_mutex_lock(&sockApi.writeMutex);
        pollNbTours(sockApi.socket, sharedInfo.trainData[0].station);
        pthread_mutex_unlock(&sockApi.writeMutex);
        while (sharedInfo.polling_tours) usleep(2000);
    }


    // initialisation de la struct partagée

    // préparation des threads et des sockets
    pthread_join(thread_socket, NULL);

    log_debug("Fermeture de la socket automate");
    close(sockApi.socket);
    log_debug("Fermeture de la socket du gestionnaire de ressources");
}


