#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "constants.h"
#include "utils.h"
#include "lib/train_parser.h"

#define MAX 80
#define PORT 8080
#define SA struct sockaddr


int initSocket(int sockfd, char *addr, int port);

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

/**
 *
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 * @param addr l'adresse du segment mémoire à écrire
 * @param size le nombre d'éléments à écrire
 * @param data le tableau contenant les éléments à écrire
 */
void write_internal_word(int sockfd, int addr, int size, unsigned int *data) {
    puts("\n\n-----------WRITE-----------\n\n");
    tramexway_t tramexway;
    prefil_trame_3niveaux(&tramexway, UNITE_WRITE_OBJECT);
    tramexway.trame[7] = 0x68; // segment des numériques
    tramexway.trame[8] = 0x07; // type mot interne
    add_two_bites_variable(&tramexway, 9, addr);
    add_two_bites_variable(&tramexway, 11, size);
    tramexway.length += 4;
    for (int i = 0; i < size; ++i) {
        add_two_bites_variable(&tramexway, (int) tramexway.length, (int) data[i]);
        tramexway.length += 2;
    }
    printf("\nSent bytes :");
    print_hex_array(tramexway);
    send_request(sockfd, &tramexway);
    // nouveau buffer pour lire le CR de l'automate
    unsigned char readBuf[MAX];
    memset(readBuf, 0, MAX);
    read(sockfd, readBuf, MAX);
    printf("read bytes :");
    for (int i = 7; i < 16; i++) {
        printf("%X ", readBuf[i]);
    }
    puts("\n\n--------END WRITE----------\n\n");
}

/**
 * Lit un tableau de mots à partir d'une adresse passée en paramètre
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 * @param addr l'adresse du segment mémoire à lire
 * @param size le nombre d'éléments à lire
 */
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
    REPONSE[1] = MESSAGE_BUFFER[3]; // addr pc
    REPONSE[2] = MESSAGE_BUFFER[4]; //
    REPONSE[3] = MESSAGE_BUFFER[1]; // addr automate
    REPONSE[4] = MESSAGE_BUFFER[2];
    REPONSE[5] = 0x19; // 5 niveaux
    REPONSE[6] = MESSAGE_BUFFER[6];
    REPONSE[7] = 0xFE; // cr ok

    unsigned char requete[MAX];
    encapsulation(REPONSE, requete, 8);
    write(sockfd, requete, 15);
}

void alimenter_troncon(int sockfd, int nTroncon) {
    unsigned int data[1] = {nTroncon};
    write_internal_word(sockfd, 50, sizeof(data) / sizeof(unsigned int), data);
    printf("\n\n ------LISTENING TO API----- \n\n");
    unsigned char MODBUS_BUFFER[7]; // Packet modbus
    memset(MODBUS_BUFFER, 0, 7);
    read(sockfd, MODBUS_BUFFER, 7);
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

    printf("CODE REQUETE : %d", MESSAGE_BUFFER[7]);
    printf("ADRESSE : %d", MESSAGE_BUFFER[11] + (MESSAGE_BUFFER[12] << 8));
    printf("QUANTITE : %d", MESSAGE_BUFFER[13] + (MESSAGE_BUFFER[14] << 8));
    printf("VALEUR !!! : %d", MESSAGE_BUFFER[15] + (MESSAGE_BUFFER[16] << 8));
}

void askRessource(int gestSock, int nbRessources, const int *ressources) {
    int tailleMessage = nbRessources + 3;
    unsigned char REQUEST_BUFFER[tailleMessage];
    REQUEST_BUFFER[0] = 1;//id du train
    REQUEST_BUFFER[1] = (unsigned char) nbRessources;
    REQUEST_BUFFER[2] = 1; // prise de ressources
    for (int i = 0; i < nbRessources; ++i) {
        REQUEST_BUFFER[3 + i] = (int) ressources[i];
    }
    for (int i = 0; i < tailleMessage; ++i) {
        printf("[%d] = %X", i, REQUEST_BUFFER[i]);
    }
    printf("\n");
    write(gestSock, REQUEST_BUFFER, tailleMessage);

    read(gestSock, REQUEST_BUFFER, tailleMessage);
    for (int i = 0; i < tailleMessage; ++i) {
        printf("%X", REQUEST_BUFFER[i]);
    }
    fflush(stdout);
}


void train1(int sockAPI, int sockGEST) {
    puts("Train1");
    usleep(12500);

    FILE *train1File;
    train1File = fopen("./trains/train1.txt", "r+");
    if (train1File == NULL) {
        puts("Couldn't open file, exiting...");
        return;
    }

    trainSequence_t *trainSequence = parseTrainSequence(train1File);
    printf("Train_id = %d", trainSequence->train_id);
    // prendre ressource r1 r2
//    int ressources[2] = {1,2};
//    puts("DEMANDE DE RESSOURCES !!!");
//    askRessource(sockGEST, 2, ressources);
    // demander aiguillage A1 -> biais
    // demander allumage TI0 -> cr C0
    // demander aiguillage tj0 -> biais
    // demander aiguillage A0 -> biais
    // demander allumage TJ1 -> cr C2
    // rendre R1
    // ecouter cr -> C1
    // rendre R2
    // demander allumage T11 -> cr 20
    // prendre ressource r3 r4
    // demander aiguillage A7 -> biais
    // demander aiguillage TJ1 -> biais
    // demander allumage t16 -> cr 32
    // rendre R3
    // demander inversion 50ms TI6 -> cr ok
    // prendre R3 et R5
    // demander allumage T16 -> cr 32
    // rendre R4
    // ecouter cr -> c31
    // rendre R3
    // demander R6
    // demander allumage t17 -> cr 21
    // rendre R5
    // demander R1 et R2
    // demander aiguillage tj0 -> biais
    // demander aiguillage A0 -> biais
    // demander allumage t12 -> cr 6
    // rendre R6
    // ecouter cr 2
    // rendre R2
    // demander aiguillage A1 -> biais
    // demander allumage ti1 -> cr0
    // Rendre R1
    // demander inversion 50ms TI0 -> cr ok

}


int main(int argc, char *argv[]) {

    int sockfd = 1;
//    sockfd = initSocket(sockfd, "10.22.205.202", 502);

    int sockGest = 1;
//    sockGest = initSocket(sockGest, "127.0.0.1", 8080);
    train1(sockfd, sockGest);
//    stop(sockfd);
//    getchar();
//    run(sockfd);
//    unsigned int data[3] = {0x2048, 0x1024, 0xffff};
//    write_internal_word(sockfd, 100, sizeof(data) / sizeof(unsigned int), data);
//    unsigned int data2[1] = {14};
//    write_internal_word(sockfd, 50, 1, data2);
//    read_internal_word(sockfd, 100, 3);
//    usleep(500);
//    listen_to_api(sockfd);
    getchar();
//    close(sockfd);
//    close(sockGest);
}

int initSocket(int sockfd, char *addr, int port) {
    struct sockaddr_in servaddrAPI;

    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else
        printf("Socket successfully created..\n");
    bzero(&servaddrAPI, sizeof(servaddrAPI));

    // assign IP, PORT
    servaddrAPI.sin_family = AF_INET;
//    servaddrAPI.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddrAPI.sin_addr.s_addr = inet_addr(addr);
    servaddrAPI.sin_port = htons(port);

    // connect the client socket to server socket
    if (connect(sockfd, (SA *) &servaddrAPI, sizeof(servaddrAPI)) != 0) {
        printf("connection with the server failed...\n");
        //exit(0);
    } else
        printf("connected to the server..\n");
    return sockfd;
}
