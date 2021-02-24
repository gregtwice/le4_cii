#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "constants.h"
#include "utils.h"

#define MAX 80
#define PORT 8080
#define SA struct sockaddr



int initSocket(int sockfd);

/**
 * Cette fonction permet d'ajouter le préfixe MODBUS au message en ajoutant les infos et la taille du message
 * @param NPDU Le tableau avec la requête NPDU XWAY
 * @param requete le tableau qui contiendra le message à envoyer à l'automate
 * @param bufferSize la taille du tableau NPDU
 */
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

/**
 * effectue une requête RUN sur l'automate
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 */
void run(int sockfd) {
    tramexway_t tramexway;
    prefil_trame(0xF0,&tramexway,UNITE_RUN);
    send_request(sockfd,&tramexway);
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
    prefil_trame(0xF0,&tramexway,UNITE_STOP);
    send_request(sockfd,&tramexway);
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
 *
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 * @param addr l'adresse du segment mémoire à écrire
 * @param size le nombre d'éléments à écrire
 * @param data le tableau contenant les éléments à écrire
 */
void write_internal_word(int sockfd, int addr, int size, unsigned int *data) {
    unsigned char NPDUXWAY[MAX];
    memset(NPDUXWAY, 0, sizeof(NPDUXWAY));
    // xway
    NPDUXWAY[0] = 0xF0;
    NPDUXWAY[1] = STATION_EMETEUR;
    NPDUXWAY[2] = RESEAU_EMETEUR;
    NPDUXWAY[3] = FIPWAY_ID; //reseau cible
    NPDUXWAY[4] = AUTOMATE_TRAIN; //machine cible (train)
    //npdu
    NPDUXWAY[5] = UNITE_WRITE_OBJECT;
    NPDUXWAY[6] = 6;
    NPDUXWAY[7] = 0x68; // segment des numériques
    NPDUXWAY[8] = 0x07; // type mot interne

    // séparation du nombre en 2 octets
    NPDUXWAY[9] = addr & 0x00FF;
    NPDUXWAY[10] = (addr & 0xFF00) >> 8; // on doit décaler de 8 pour pouvoir rentrer dans la taille de l'uchar

    NPDUXWAY[11] = size & 0x00FF;
    NPDUXWAY[12] = (size & 0xFF00) >> 8;
    int bufferSize = 13;
    for (int i = 0; i < size; ++i) {
        NPDUXWAY[(13 + (i * 2))] = (char) (data[i] & 0x00FF);
        NPDUXWAY[(13 + (i * 2) + 1)] = (char) ((data[i] & 0xFF00) >> 8);
        bufferSize += 2;
    }
    printf("buffersize : %d\n", bufferSize);

    unsigned char requete[7 + bufferSize];
    encapsulation(NPDUXWAY, requete, bufferSize);
    printf("sent bytes :");
    for (int i = 7; i < bufferSize + 7; i++) {
        printf("%X ", requete[i]);
    }
    printf("\n");
    for (int i = 0; i < bufferSize + 7; i++) {
        printf("%X ", requete[i]);
    }
    printf("\n");
    write(sockfd, requete, sizeof(requete));
    // nouveau buffer pour lire le CR de l'automate
    unsigned char readBuf[MAX];
    memset(readBuf, 0, MAX);
    read(sockfd, readBuf, MAX);
    printf("read bytes :");
    for (int i = 7; i < 16; i++) {
        printf("%X ", readBuf[i]);
    }
}

/**
 * Lit un tableau de mots à partir d'une adresse passée en paramètre
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 * @param addr l'adresse du segment mémoire à lire
 * @param size le nombre d'éléments à lire
 */
void read_internal_word(int sockfd, int addr, int size) {
    printf("\n\n-----------READ----------\n\n");
    unsigned char NPDUXWAY[MAX];
    memset(NPDUXWAY, 0, sizeof(NPDUXWAY));
    // xway
    NPDUXWAY[0] = 0xF0;
    NPDUXWAY[1] = STATION_EMETEUR;
    NPDUXWAY[2] = RESEAU_EMETEUR;
    NPDUXWAY[3] = FIPWAY_ID; //reseau cible
    NPDUXWAY[4] = AUTOMATE_TRAIN; //machine cible (train)
    //npdu
    NPDUXWAY[5] = UNITE_READ_OBJECT;
    NPDUXWAY[6] = 6;
    NPDUXWAY[7] = 0x68; // segment des numériques
    NPDUXWAY[8] = 0x07; // type mot interne

    NPDUXWAY[9] = addr & 0x00FF;
    NPDUXWAY[10] = addr & 0xFF00;
    NPDUXWAY[11] = size & 0x00FF;
    NPDUXWAY[12] = size & 0xFF00;
    int bufferSize = 13;


    unsigned char requete[7 + bufferSize];
    encapsulation(NPDUXWAY, requete, bufferSize);
    printf("sent bytes :");
    for (int i = 7; i < bufferSize + 7; i++) {
        printf("%X ", requete[i]);
    }
    printf("\n");
    write(sockfd, requete, sizeof(requete));
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
}

/**
 * Ecoute la socket en attente d'une requête de l'automate et renvoie un cr OK
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 */
void listen_to_api(int sockfd) {
    printf("\n\n ------LISTENING TO API----- \n\n");
    unsigned char MODBUS_BUFFER[7]; // Packet modbus
    memset(MODBUS_BUFFER, 0, 7);
    read(sockfd, MODBUS_BUFFER, 7);
    printf("read bytes :");
    for (int i = 0; i < 5; i++) {
        printf("%X ", MODBUS_BUFFER[i]);
    }

    // taille du reste du message
    int message_size = MODBUS_BUFFER[5] - 1;
    printf("size = %d\n", message_size);
    unsigned char MESSAGE_BUFFER[message_size];
    memset(MESSAGE_BUFFER, 0, message_size);
    read(sockfd, MESSAGE_BUFFER, message_size);
    printf("read bytes :");
    for (int i = 0; i < message_size; i++) {
        printf("%X ", MESSAGE_BUFFER[i]);
    }

    // F1 addr automate addr pc
    unsigned char REPONSE[8];
    REPONSE[0] = MESSAGE_BUFFER[0];
    REPONSE[1] = MESSAGE_BUFFER[3]; // addr automate
    REPONSE[2] = MESSAGE_BUFFER[4]; //
    REPONSE[3] = MESSAGE_BUFFER[1];
    REPONSE[4] = MESSAGE_BUFFER[2];
    REPONSE[5] = 0x19;
    REPONSE[6] = MESSAGE_BUFFER[6];
    REPONSE[7] = 0xFE;

    unsigned char requete[MAX];
    encapsulation(REPONSE, requete, 8);
    write(sockfd, requete, 15);
}

int main(int argc, char *argv[]) {

    int sockfd =1;
    sockfd = initSocket(sockfd);


    stop(sockfd);
    getchar();
    run(sockfd);
//    unsigned int data[3] = {0x2048, 0x1024, 0xffff};
//    write_internal_word(sockfd, 100, sizeof(data), data);
//    unsigned int data2[1] = {14};
//    write_internal_word(sockfd, 50, 1, data2);
//    read_internal_word(sockfd, 100, 3);
//    usleep(500);
//    listen_to_api(sockfd);
    getchar();
    close(sockfd);
}

int initSocket(int sockfd) {
    struct sockaddr_in servaddr;

    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
//    servaddr.sin_addr.s_addr = inet_addr("10.22.205.202");
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        //exit(0);
    } else
        printf("connected to the server..\n");
    return sockfd;
}
