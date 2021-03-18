//
// Created by gregoire on 16/03/2021.
//


#include "networking.h"
#include "log.h"

#define SA struct sockaddr

int initSocket(char *addr, int port) {
    struct sockaddr_in servaddrAPI;
    int sockfd;
    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        log_error("socket creation failed...");
        exit(0);
    } else
        log_info("Socket successfully created..");
    bzero(&servaddrAPI, sizeof(servaddrAPI));

    // assign IP, PORT
    servaddrAPI.sin_family = AF_INET;
    servaddrAPI.sin_addr.s_addr = inet_addr(addr);
    servaddrAPI.sin_port = htons(port);

    // connect the client socket to server socket
    if (connect(sockfd, (SA *) &servaddrAPI, sizeof(servaddrAPI)) != 0) {
        log_error("connection with the server failed...\n");
        exit(0);
    } else
        log_debug("connected to the server..\n");
    return sockfd;
}