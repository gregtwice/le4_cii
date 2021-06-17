//
// Created by gregoire on 17/06/2021.
//


#include "lib/utils.h"
#include "lib/xway.h"
#include <stdio.h>

#define ADDR_T1_DEBUT 900
#define ADDR_T1_FIN 998


shared_info sharedInfo;


int main() {

    log_set_level(4);
    socketWrapper sw = initSocket("10.22.205.203", 502);

    FILE *ficT1 = fopen("apprentissage/train1.txt", "wt");
    int nbElements = ADDR_T1_FIN - ADDR_T1_DEBUT;

    for (int i = 0; i < nbElements; i += 20) {
        tramexway_t tramexway = read_double_word(sw.socket, ADDR_T1_DEBUT + i, 10, 51);
        log_trame(tramexway);
        int tete = 7;
        for (int j = tete; j < tramexway.length; j += 4) {
            if (((j - 7) / 4) * 2 + i <= nbElements) {

                unsigned long res = tramexway.trame[j] +
                                    (tramexway.trame[j + 1] << 8) +
                                    (tramexway.trame[j + 2] << 16) +
                                    (tramexway.trame[j + 3] << 24);
                printf("res %d = %ld\n", ((j - 7) / 4) * 2 + 900 + i, res);
                fprintf(ficT1, "%ld\n", res);
            }
        }
        sleep(1);
    }
}