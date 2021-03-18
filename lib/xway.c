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
    prefil_trame_3niveaux(&tramexway, UNITE_RUN);
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
    prefil_trame_3niveaux(&tramexway, UNITE_STOP);
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


void write_internal_word(int sockfd, int addr, int size, unsigned int *data) {
    log_trace("Ecriture vers l'automate");
    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_WRITE_OBJECT);
    tramexway.trame[7] = 0x68; // segment des numériques
    tramexway.trame[8] = 0x07; // type mot interne
    tramexway.length += 2;
    add_two_bites_variable(&tramexway, 9, addr);
    add_two_bites_variable(&tramexway, 11, size);
    for (int i = 0; i < size; ++i) {
        add_two_bites_variable(&tramexway, (int) tramexway.length, (int) data[i]);
    }
    log_trame(tramexway);
    send_request(sockfd, &tramexway);
    // nouveau buffer pour lire le CR de l'automate
    unsigned char readBuf[MAX];
    memset(readBuf, 0, MAX);
    tramexway = *read_xway(sockfd);
    log_trame(tramexway);
    log_trace("Ecriture terminée");
}


void read_internal_word(int sockfd, int addr, int size) {
    printf("\n\n-----------READ----------\n\n");
    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_READ_OBJECT);
/*    unsigned char NPDUXWAY[MAX];
    memset(NPDUXWAY, 0, sizeof(NPDUXWAY));
     xway
    NPDUXWAY[0] = 0xF0;
    NPDUXWAY[1] = STATION_EMETEUR;
    NPDUXWAY[2] = RESEAU_EMETEUR;
    NPDUXWAY[3] = FIPWAY_ID; //reseau cible
    NPDUXWAY[4] = AUTOMATE_TRAIN; //machine cible (train)
    npdu
    NPDUXWAY[5] = UNITE_READ_OBJECT;
    NPDUXWAY[6] = 6;
    NPDUXWAY[7] = 0x68; // segment des numériques
    NPDUXWAY[8] = 0x07; // type mot interne

    NPDUXWAY[9] = addr & 0x00FF;
    NPDUXWAY[10] = addr & 0xFF00;
    NPDUXWAY[11] = size & 0x00FF;
    NPDUXWAY[12] = size & 0xFF00;*/
    int bufferSize = 13;
    tramexway.trame[7] = 0x68;
    tramexway.trame[8] = 0x07;
    tramexway.length += 2;
    add_two_bites_variable(&tramexway, 9, addr);
    add_two_bites_variable(&tramexway, 11, size);

/*    unsigned char requete[7 + bufferSize];
    encapsulation(NPDUXWAY, requete, bufferSize);
    printf("sent bytes :");
    for (int i = 7; i < bufferSize + 7; i++) {
        printf("%X ", requete[i]);
    }
    printf("\n");
    write(sockfd, requete, sizeof(requete));*/
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

int wait_api_action(int sockfd) {
    log_trace("En attente du filtre de commande");
    tramexway_t *received_trame = read_xway(sockfd);
    log_trame(*received_trame);
    tramexway_t tramexway;
    prefil_trame_5niveaux(&tramexway, received_trame->trame);
    log_trame(tramexway);
    log_trace("Write Var Reçu... Envoi du CR");
    send_request(sockfd, &tramexway);
    log_trace("Envoi Terminé");
    int cr = received_trame->trame[6] + ((int) (received_trame->trame[7]) << 8);
    return cr;
}

tramexway_t *read_xway(int sockfd) {
    static tramexway_t tramexway;
    unsigned char MODBUS_BUFFER[7];
    read(sockfd, MODBUS_BUFFER, 7);
    int trame_size = MODBUS_BUFFER[5] - 1;
    unsigned char XWAY_BUFFER[trame_size];
    read(sockfd, XWAY_BUFFER, trame_size);
    memcpy(tramexway.trame, XWAY_BUFFER, trame_size);
    tramexway.length = trame_size;
    return &tramexway;
}
