//
// Created by gregoire on 23/02/2021.
//

#include "utils.h"

static void modbus_encapsulate(tramexway_t tramexway, unsigned char *resquest_array) {
    resquest_array[0] = 0;
    resquest_array[1] = 0;
    resquest_array[2] = 0;
    resquest_array[3] = 1;
    resquest_array[4] = 0;
    resquest_array[5] = (unsigned char) (tramexway.length + 1);
    resquest_array[6] = 0;
    for (int i = 0; i < tramexway.length; i++) {
        resquest_array[7 + i] = tramexway.trame[i];
    }
}

void send_request(int socket, tramexway_t *tramexway) {
    unsigned char requete[7 + tramexway->length];
    modbus_encapsulate(*tramexway, requete);
    write(socket, requete, sizeof(requete));
}


void private_log_trame(tramexway_t tramexway, char *file, int line) {
    char buffer[tramexway.length * 3];
    buffer[0] = '\0';
    char temp[3];
    for (int i = 0; i < tramexway.length; ++i) {
        sprintf(temp, "%X ", tramexway.trame[i]);
        strncat(buffer, temp, 3);
    }
    log_log(LOG_TRACE, file, line, "Trame len = %d : %s ", tramexway.length, buffer);
}

void print_hex_array(tramexway_t tramexway) {
    log_trame(tramexway);
}

void prefil_trame_3niveaux(tramexway_t *tramexway, unsigned char request_code, int station) {
    memset(tramexway->trame, 0, MAX_REQUEST_LENGTH);
    train_data *data = findTrainData(station);
    tramexway->trame[0] = 0xF0;
    tramexway->trame[1] = station;
    tramexway->trame[2] = (data->trainConfig->train_reseau << 4) + data->trainConfig->train_porte;
    tramexway->trame[3] = sharedInfo.trainConfig.automate_station; //reseau cible
    tramexway->trame[4] = (sharedInfo.trainConfig.automate_reseau << 4) + sharedInfo.trainConfig.automate_porte; //machine cible (train)
    tramexway->trame[5] = request_code;
    tramexway->trame[6] = 6;
    tramexway->length = 7;
}


void prefil_trame_5niveaux(tramexway_t *tramexway, const unsigned char *API_REQUEST) {
    memset(tramexway->trame, 0, MAX_REQUEST_LENGTH);
    tramexway->trame[0] = API_REQUEST[0];
    tramexway->trame[1] = API_REQUEST[3]; // addr automate
    tramexway->trame[2] = API_REQUEST[4];
    tramexway->trame[3] = API_REQUEST[1]; // addr pc
    tramexway->trame[4] = API_REQUEST[2];
    tramexway->trame[5] = 0x19; // 5 niveaux
    tramexway->trame[6] = API_REQUEST[6];
    tramexway->trame[7] = 0xFE; // cr ok
    tramexway->length = 8;
}

void add_two_bytes_variable(tramexway_t *tramexway, int index, int value) {
    tramexway->trame[index] = value & 0x00FF;
    tramexway->trame[index + 1] = (value >> 8);
    tramexway->length += 2;
}

void parseTrainConfig(char *filename, train_config *config) {
    log_info("Parsage du fichier de configuration du train");
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        log_error("Impossible d'ouvrir le fichier de configuration");
        exit(-1);
    }
    int buffer_len = 80;
    char buffer[buffer_len];
    char param[buffer_len];
    char value[buffer_len];
    while (!feof(fp)) {
        fgets(buffer, buffer_len, fp);
        if (strlen(buffer) == 1) continue;
        sscanf(buffer, "%s = %s", param, value);
        int valueInt;

        if (strchr(value, '.') == NULL) {
            sscanf(value, "%d", &valueInt);
        }
        if (strcmp(param, "TRAIN_STATION") == 0)
            config->train_station = valueInt;
        if (strcmp(param, "TRAIN_RESEAU") == 0)
            config->train_reseau = valueInt;
        if (strcmp(param, "TRAIN_PORTE") == 0)
            config->train_porte = valueInt;
        if (strcmp(param, "AUTOMATE_IP") == 0)
            strcpy(config->automate_ip, value);
        if (strcmp(param, "AUTOMATE_PORT") == 0)
            config->automate_port = valueInt;
        if (strcmp(param, "AUTOMATE_STATION") == 0)
            config->automate_station = valueInt;
        if (strcmp(param, "AUTOMATE_RESEAU") == 0)
            config->automate_reseau = valueInt;
        if (strcmp(param, "AUTOMATE_PORTE") == 0)
            config->automate_porte = valueInt;
        if (strcmp(param, "GESTIONNAIRE_IP") == 0)
            strcpy(config->gestionnaire_ip, value);
        if (strcmp(param, "GESTIONNAIRE_PORT") == 0)
            config->gestionnaire_port = valueInt;
        if (strcmp(param, "LOG_LEVEL") == 0)
            config->log_level = valueInt;
        if (strcmp(param, "LOOP") == 0) {
            if (valueInt == -1) {
                config->loop = 1;
            } else {
                config->loop = 0;
                config->nbTours = valueInt;
            }
        }
    }
}

train_data *findTrainData(int station) {
    for (int i = 0; i < 2; ++i) {
        if (sharedInfo.trainData[i].station == station) {
            return &sharedInfo.trainData[i];
        }
    }
    return NULL;
}
