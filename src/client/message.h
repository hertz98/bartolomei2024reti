#pragma once
#include "stdint.h"

typedef struct Message {
    void * data; // Puntatore al contenuto del messaggio da inviare
    uint32_t lenght; // Dimensione del messaggio (compreso di caratteri speciali)
    int transmitted; // Numero di bytes inviati
    bool toFree; // Indica se data deve essere deallocato dopo l'invio
    struct Message *next; // Prossimo messaggio (se esiste)
} Message;