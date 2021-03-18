//
// Created by gregoire on 16/03/2021.
//

#ifndef CII_XWAY_H
#define CII_XWAY_H

#include "utils.h"
#include "log.h"


#define UNITE_RUN 0x24
#define UNITE_STOP 0x25
#define UNITE_WRITE_OBJECT 0x37
#define UNITE_READ_OBJECT 0x36


/**
 * Cette fonction permet d'ajouter le préfixe MODBUS au message en ajoutant les infos et la taille du message
 * @param NPDU Le tableau avec la requête NPDU XWAY
 * @param requete le tableau qui contiendra le message à envoyer à l'automate
 * @param bufferSize la taille du tableau NPDU
 */
void encapsulation(unsigned char *NPDU, unsigned char *requete, int bufferSize);

/**
 * effectue une requête RUN sur l'automate
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 */
void run(int sockfd);

/**
 * effectue une requête STOP sur l'automate
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 */
void stop(int sockfd);

/**
 *
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 * @param addr l'adresse du segment mémoire à écrire
 * @param size le nombre d'éléments à écrire
 * @param data le tableau contenant les éléments à écrire
 */
void write_internal_word(int sockfd, int addr, int size, unsigned int *data);

/**
 *
 * Lit un tableau de mots à partir d'une adresse passée en paramètre
 * @param sockfd la socket tcp permettant de dialoguer avec l'automate
 * @param addr l'adresse du segment mémoire à lire
 * @param size le nombre d'éléments à lire
 */
void read_internal_word(int sockfd, int addr, int size);

/**
 * Attend un Cr de la part de l'automate après une action
 * @param sockfd
 * @return
 */
int wait_api_action(int sockfd);

/**
 * Fonction qui attend un cr de l'automate
 * @param sockfd
 * @return
 */
tramexway_t *read_xway(int sockfd);

#endif //CII_XWAY_H
