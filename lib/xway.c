//
// Created by gregoire on 16/03/2021.
//

#include "xway.h"




void encapsulation(unsigned char *NPDU, unsigned char *requete, int bufferSize) {
    bzero(requete, sizeof(requete));
    requete[0] = 0;
    requete[1] = 0;
    requete[2] = 0;
    requete[3] = 1;
    requete[4] = 0;
    requete[5] = bufferSize + 1;
    requete[6] = 0;
    for (int i = 0; i < bufferSize; i++) {
        requete[7 + i] = NPDU[i];
    }
}


void run(int sockfd) {
    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_RUN, 0);
    send_request(sockfd, &tramexway);
    printf("sent bytes :");
    print_hex_array(tramexway);
    printf("\n");

    unsigned char readBuf[MAX];
    memset(readBuf, 0, MAX);
    read(sockfd, readBuf, MAX);
    printf("read bytes :");
    for (int i = 7; i < 13; i++) {
        printf("%X ", readBuf[i]);
    }
}

/**
 * effectue une requête STOP sur l'automate
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 */
void stop(int sockfd) {

    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_STOP, 0);
    send_request(sockfd, &tramexway);
    printf("sent bytes :");
    print_hex_array(tramexway);
    printf("\n");

    unsigned char readBuf[MAX];
    memset(readBuf, 0, MAX);
    read(sockfd, readBuf, MAX);
    printf("read bytes :");
    for (int i = 7; i < 13; i++) {
        printf("%X ", readBuf[i]);
    }
}


void write_internal_word(int sockfd, int addr, int size, unsigned int *data, int station) {
    log_trace("%d : Ecriture vers l'automate", station);
    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_WRITE_OBJECT, station);
    tramexway.trame[7] = 0x68; // segment des numériques
    tramexway.trame[8] = 0x07; // type mot interne
    tramexway.length += 2;
    add_two_bytes_variable(&tramexway, 9, addr);
    add_two_bytes_variable(&tramexway, 11, size);
    for (int i = 0; i < size; ++i) {
        add_two_bytes_variable(&tramexway, (int) tramexway.length, (int) data[i]);
    }
    log_trame(tramexway);
    send_request(sockfd, &tramexway);
    train_data *trainData = findTrainData(station);
    // nouveau buffer pour lire le CR de l'automate
    unsigned char readBuf[MAX];
    memset(readBuf, 0, MAX);
//    tramexway = *read_xway(sockfd);
    trainData->turn = 2;
    wait_response(trainData);

    log_trace("%d : Ecriture terminée", station);
}

void wait_response(train_data *data) {
    while (data->turn != 1)
        usleep(300);
}

int pollRun(int sockfd, int station) {

    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_READ_OBJECT, station);

    tramexway.trame[7] = 0x68;
    tramexway.trame[8] = 0x07;
    tramexway.length += 2;
    add_two_bytes_variable(&tramexway, 9, 160);
    add_two_bytes_variable(&tramexway, 11, 1);
    log_trame(tramexway);
    send_request(sockfd, &tramexway);

    return 0;
}


tramexway_t read_double_word(int sockfd, int addr, int size, int station) {
    tramexway_t tramexway;

    memset(tramexway.trame, 0, MAX_REQUEST_LENGTH);
    tramexway.trame[0] = 0xF0;
    tramexway.trame[1] = station;
    tramexway.trame[2] = (8 << 4);
    tramexway.trame[3] = 23; //reseau cible
    tramexway.trame[4] = (8 << 4) + 0; //machine cible (train)
    tramexway.trame[5] = UNITE_READ_OBJECT;
    tramexway.trame[6] = 6;
    tramexway.length = 7;


    tramexway.trame[7] = 0x68;
    tramexway.trame[8] = 0x08;
    tramexway.length += 2;

    add_two_bytes_variable(&tramexway, 9, addr);
    add_two_bytes_variable(&tramexway, 11, size);

    print_hex_array(tramexway);
    send_request(sockfd, &tramexway);

    tramexway_t *rcv = read_xway(sockfd);


    return *rcv;
}

void read_internal_word(int sockfd, int addr, int size, int station) {
    printf("\n\n-----------READ----------\n\n");
    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_READ_OBJECT, station);

    int bufferSize = 13;
    tramexway.trame[7] = 0x68;
    tramexway.trame[8] = 0x07;
    tramexway.length += 2;
    add_two_bytes_variable(&tramexway, 9, addr);
    add_two_bytes_variable(&tramexway, 11, size);

    print_hex_array(tramexway);
    send_request(sockfd, &tramexway);

    unsigned char MODBUS_BUFFER[7];
    memset(MODBUS_BUFFER, 0, 7);
    read(sockfd, MODBUS_BUFFER, 7);
    printf("read bytes :");
    for (int i = 0; i < 6; i++) {
        printf("%X ", MODBUS_BUFFER[i]);
    }
    int message_size = MODBUS_BUFFER[5] - 1;
    printf("size = %d\n", message_size);
    // tableau de taille variable contenant le reste du CR sans la trame MODBUS
    unsigned char MESSAGE_BUFFER[message_size];
    memset(MESSAGE_BUFFER, 0, message_size);
    read(sockfd, MESSAGE_BUFFER, message_size);
    printf("read bytes :");
    for (int i = 0; i < message_size; i++) {
        printf("%X ", MESSAGE_BUFFER[i]);
    }
    printf("\n\n--------END READ--------\n\n");

}

int wait_api_action(socketWrapper *sock, int station) {
    // le train se met en attente d'un write var de l'automate
    pthread_mutex_lock(sharedInfo.accessMutex);
    train_data *data = findTrainData(station);
    data->turn = 2;
    pthread_mutex_unlock(sharedInfo.accessMutex);
    log_debug("%s : En attente du filtre de commande", data->trainName);

    // boucle d'attente
    wait_response(data);

    // le train à reçu le write var, il peut répondre et continuer son traitement
    tramexway_t received_trame = data->lastReceived;
    tramexway_t tramexway;
    prefil_trame_5niveaux(&tramexway, received_trame.trame);
    log_trace("%s : Write Var Reçu... Envoi du CR", data->trainName);
    log_debug("%s : Trame à renvoyer : ", data->trainName);
    log_trame(tramexway);
    pthread_mutex_lock(&sock->writeMutex);
    send_request(sock->socket, &tramexway);
    pthread_mutex_unlock(&sock->writeMutex);
    log_trace("Envoi Terminé");
    int cr = received_trame.trame[received_trame.length - 2] + ((int) (received_trame.trame[received_trame.length - 1]) << 8);
    return cr;
}

tramexway_t *read_xway(int sockfd) {
    static tramexway_t tramexway;
    unsigned char MODBUS_BUFFER[7];
    log_debug("Reading socket");
    read(sockfd, MODBUS_BUFFER, 7);
    int trame_size = MODBUS_BUFFER[5] - 1;
    unsigned char XWAY_BUFFER[trame_size];
    read(sockfd, XWAY_BUFFER, trame_size);
    memcpy(tramexway.trame, XWAY_BUFFER, trame_size);
    tramexway.length = trame_size;
    return &tramexway;
}

int pollNbTours(int sockfd, int station) {
    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_READ_OBJECT, station);
    tramexway.trame[7] = 0x68;
    tramexway.trame[8] = 0x07;
    tramexway.length += 2;
    add_two_bytes_variable(&tramexway, 9, 700);
    add_two_bytes_variable(&tramexway, 11, 4);
    log_trame(tramexway);
    send_request(sockfd, &tramexway);
    return 0;
}

