#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<pthread.h>
#include <signal.h>

pthread_mutex_t ressources[9];
int MAX_BUF_LEN = 20;
int se;

void closeSock(){
    puts("Closing socket");
    close(se);
}


void *connexionHandler(void *socket_ptr) {
    puts("Processing train");
    int sock = *(int *) socket_ptr;
    unsigned char BUFFER[MAX_BUF_LEN];
    while (1) {
        puts("Attente message !!!");
        read(sock, BUFFER, MAX_BUF_LEN);
        puts("Message recu !!!");
        // buffer [0] = id du train
        // buffer [1] = nbRessources
        // buffer [2] = 1 ? prise : 2 res rendue
        // buffer [3..] = ressources
        unsigned char train_id = BUFFER[0];
        unsigned char nb_ressources = BUFFER[1];
        if (BUFFER[2] == 1) {
            for (int i = 0; i < (int) nb_ressources; ++i) {
                printf("Le train [%d] PREND la ressource %d\n", train_id, (int) BUFFER[i + 3]);
                pthread_mutex_lock(&ressources[(int) BUFFER[i + 3]-1]);
            }
        } else {
            for (int i = 0; i < (int) nb_ressources; ++i) {
                printf("Le train [%d] REND la ressource %d\n", train_id, (int) BUFFER[i + 3] );
                pthread_mutex_unlock(&ressources[(int) BUFFER[i + 3]-1]);
            }
        }
        // ok -> on lui renvoie son buffer
        if (write(sock, BUFFER, nb_ressources + 3) == 0) {
            break;
        }
    }

    return 0;
}


int main() {
    signal(SIGTERM, closeSock);
    signal(SIGINT, closeSock);
    atexit(closeSock);
    for (int i = 0; i < 9; ++i) {
        pthread_mutex_init(&ressources[i], NULL);
    }
    int  sd;
    struct sockaddr_in svc, clt;
    socklen_t cltLen;
// Création de la socket de réception d’écoute des appels
    se = socket(PF_INET, SOCK_STREAM, 0);
    svc.sin_family = PF_INET;
    svc.sin_port = htons(8080);
    svc.sin_addr.s_addr = INADDR_ANY;
    memset(&svc.sin_zero, 0, 8);
    pthread_t threadAPI;
    if (bind(se, (struct sockaddr *) &svc, sizeof(svc)) != 0) {
        printf("Can't bind\n");
        exit(0);
    }
    listen(se, 4);
    while (1) {
        cltLen = sizeof(clt);
        sd = accept(se, (struct sockaddr *) &clt, &cltLen);
        if (sd == -1) {
            printf("Can't accept!\n");
            break;
        }
        int err = pthread_create(&threadAPI, NULL, connexionHandler, (void *) &sd);
        if (err != 0) {
            perror("could not create thread");
            return 1;
        }
        puts("Nouveau Train Connecté !!!\n");

    }
    close(se);

}