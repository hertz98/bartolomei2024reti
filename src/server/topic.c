#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h> 
#include <stdio.h>
#include "topic.h"

bool topicsLoader(struct TopicsContext * context)
{
    int count = 0;

    DIR * dir_resource = opendir(context->directory);

    struct dirent *directory;

    return false;
}

bool topicLoad(FILE * file, struct Topic *)
{
    // Scorro il file e mi ricavo il numero di domande/risposte valide
    // creo un buffer di dimensione QUESTION_MAX_SIZE che utilizzerò come locazione temporanea per la lettura del file
    // Creo lo array e riapro il file
    //foreach(riga){
        // scrivo la domanda/riposta nel buffer
        // malloc e la aggiungo alla struttura
        // se struttura completa, la aggiungo all'array o alla lista
    return false;
}

void topicsFree(struct Question ** topics)
{

    return;
}
