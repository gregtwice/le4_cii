#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

int MAX_BUF_LEN = 20;
int se;

typedef struct {
    unsigned char train_id;
    pthread_mutex_t r;
} ressource;

ressource ressourcesTrain[9];

void closeSock() {
    puts("Closing socket");
    close(se);
}


void *connexionHandler(void *socket_ptr) {
    puts("Processing train");
    int sock = *(int *) socket_ptr;
    unsigned char BUFFER[MAX_BUF_LEN];
    char train_id = -1;
    while (1) {
        puts("Attente message !!!");
        int n = read(sock, BUFFER, MAX_BUF_LEN);
        if (n == 0) {
            if (train_id == -1) return NULL;
            printf("Libération des ressources du train %d\n", train_id);
            for (int i = 0; i < 9; ++i) {
                if (ressourcesTrain[i].train_id == train_id) {
                    pthread_mutex_unlock(&ressourcesTrain[i].r);
                }
            }
            return NULL;
        }
        puts("Message recu !!!");
        // buffer [0] = id du train
        // buffer [1] = nbRessources
        // buffer [2] = 1 ? prise : 2 res rendue
        // buffer [3..] = ressources
        train_id = (char) BUFFER[0];
        unsigned char nb_ressources = BUFFER[1];
        if (BUFFER[2] == 1) {
            for (int i = 0; i < (int) nb_ressources; ++i) {
                printf("Le train [%d] PREND la ressource %d\n", train_id, (int) BUFFER[i + 3]);
                pthread_mutex_lock(&ressourcesTrain[(int) BUFFER[i + 3] - 1].r);
                ressourcesTrain[(int) BUFFER[i + 3] - 1].train_id = BUFFER[0];
            }
        } else if (BUFFER[2] == 2) {
            for (int i = 0; i < (int) nb_ressources; ++i) {
                printf("Le train [%d] LIBERE la ressource %d\n", train_id, (int) BUFFER[i + 3]);
                pthread_mutex_unlock(&ressourcesTrain[(int) BUFFER[i + 3] - 1].r);
                ressourcesTrain[(int) BUFFER[i + 3] - 1].train_id = 0;
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

        pthread_mutex_init(&ressourcesTrain[i].r, NULL);
        ressourcesTrain[i].train_id = 0;
    }
    int sd;
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