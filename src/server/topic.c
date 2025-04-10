#define _DEFAULT_SOURCE
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h> 
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include "topic.h"
#include "util.h"

bool directoryCreate(char * buffer_path, char * directory)
{
    int endline = strlen(buffer_path);
    strncat(buffer_path, directory, NAME_MAX);

    if (mkdir(buffer_path, 0755) < 0)
        if (errno != EEXIST) // Se la directory esiste già perfetto così
        {
            perror("Errore nella creazione delle directory");
            return false;
        }

    buffer_path[endline] = '\0'; // Riporto il percorso a com'era prima
    return true;
}

bool topicsInit(TopicsContext *context)
{
    context->nTopics = 0;
    context->topics = NULL;

    if (!(executablePath(context->directory)))
        return false;

    if (!parentDirectory(context->directory)) // Tolgo il nome dell'eseguibile dal percorso
        return false;

    strcat(context->directory, DATA_DIR); // Aggiungo la cartella data
    
    // E provo a creare ciascuna delle directory
    if (!directoryCreate(context->directory, "") ||
        !directoryCreate(context->directory, TOPICS_DIR) ||
        !directoryCreate(context->directory, USERS_DIR))
        return false;

    return true;
}

/* topic.txt
 *
 * Domanda 1
 * Risposta 1
 * 
 * Domanda 2
 * Risposta 2
 * Domanda 3
 * Risposta 3
 * 
 * Domanda non valida
 * 
 * Risposta non valida 
 */
bool topicsLoader(TopicsContext *context)
{
    int endline = strlen(context->directory);

    strncat(context->directory, TOPICS_DIR, NAME_MAX);

    DIR * stream = opendir(context->directory);
    struct dirent *file;
    if (stream)
    {
        while ((file = readdir(stream)) != NULL)
        {
            if (file->d_type != DT_REG)
                continue;

            context->nTopics++;
            // Accetto di fare la realloc ad ogni iterazione, e solo per la inizializzazione
            context->topics = realloc(context->topics, sizeof(Topic) * context->nTopics);
            if (!context->topics)
                return false;

            Topic * current = &context->topics[context->nTopics - 1];
            memset(current, 0, sizeof(Topic));

            strncpy(current->name, file->d_name, NAME_MAX - 1);
            // current->name[ sizeof(current->name) - 1 ] = '\0';

            current->questions = NULL;
            current->nQuestions = 0;
        }
        closedir(stream);
    }
    else
        return false;

    // Ordino, non è detto che legga gli elementi della cartella in maniera ordinata
    qsort(context->topics, context->nTopics, sizeof(Topic), topics_compare);

    // Preparo un messageArray con i nomi dei topics, pronta per essere copiata e inviata
    context->topicsString = messageArray(context->nTopics + 1);
    if (!context->topicsString)
        return false;

    for (int i = 0; i < context->nTopics; i++)
    {
        if (!topicLoad(context->directory, &context->topics[i])) // Carico le singole domande
            return false;
        topic_name(context->topics[i].name); // Rimuovo numeri e punteggiatura iniziale
        messageString(&context->topicsString->messages[i], context->topics[i].name, false); // Continuo a costruire il MessageArray
    }

    context->directory[endline] = '\0'; // Riporto il percorso a quello originale

    return true;
}

bool topicLoad(char * path, Topic * topic)
{
    FILE *file;

    int endline = strlen(path);
    strncat(path, topic->name, NAME_MAX); // Qua mi serve ancora il nome del file completo

    if (!(file = fopen(path, "r")))
        return false;

    char * current_question = NULL; // Ultima domanda a cui assegnare la risposta
    char *line = NULL;
    size_t alloc_len = 0;
    while(getline(&line, &alloc_len, file) != -1)
    {
        // Ignora linee vuote, valutazione in corto circuito
        if ((line[0] == '\n' || // Linee vuote
                (line[0] == '\r' && line[1] == '\n'))) // caso Windows, valutazione in corto circuito
        {
            if (current_question) // dealloca
                free(current_question);
            current_question = NULL;
            continue;
        }

        newlineReplace(line);

        if (!current_question) // Nuova domanda
        {
            current_question = line;
        }
        else
        {
            // Costruisco la struttura dati
            Question * new_question = malloc( sizeof(Question) );
            if (!new_question)
                return false; // Memoria esaurita? Inutile continuare
            
            new_question->question = current_question; // Riprendo la domanda alla linea precedente
            new_question->answer = line;
            if (!list_append(&topic->questions, new_question)) // Costruisco la lista di domande
                return false;
            
            current_question = NULL; // Prossima domanda (non deallocare la corrente)
            topic->nQuestions++;
        }
        
        line = NULL;
    }

    if (current_question)
        free(current_question);
    free(line);
    fclose(file);

    path[endline] = '\0'; // Riporto la domanda com'era

    return true;
}

void topics_questionDestroy(void * data)
{
    if (!data)
        return;
    Question * question = (Question *) data;
    if (question->question)
        free(question->question);
    if (question->answer)
        free(question->answer);
    free(question);
}

void topic_name(char *name)
{
    if (!name[0])
        return;

    removeExtension(name);

    removeNumbering(name);

    stringStrip(name);

    name[0] = toupper( (uint8_t) name[0]); // Capitalize

    return;
}

void topicsFree(TopicsContext *context)
{
    if (!context->nTopics)
        return;

    for (int i = 0; i < context->nTopics; i++)
        list_destroy(context->topics[i].questions, topics_questionDestroy);
    
    messageArrayDestroy(&context->topicsString);

    free(context->topics);
    
    context->nTopics = 0;
    return;
}

// Parto da un array di topic tutti giocati e li rimuovo man mano che li trovo nel file del giocatore
bool *topicsUnplayed(TopicsContext *context, char *user)
{
    int endline = strlen(context->directory);
    
    char *path = context->directory;
    strcat(path, USERS_DIR);

    // Rendo il nome minuscolo
    char tmp[CLIENT_NAME_MAX + 1];
    strncpy(tmp, user, sizeof(tmp));
    stringLower(tmp);
    strcat(path, tmp);

    strcat(path, ".txt");

    bool *unplayed = (bool *) malloc(context->nTopics); // array dei topic non giocati
    for (int i = 0; i < context->nTopics; i++)
        unplayed[i] = true;

    FILE *file;
    if ((file = fopen(path, "r")))
    {
        char *line = NULL;
        size_t alloc_len = 0;
        while(getline(&line, &alloc_len, file) != -1)
        {
            if ((line[0] == '\n' || // Linee vuote
                    (line[0] == '\r' && line[1] == '\n'))) // caso Windows, valutazione in corto circuito
                continue;

            newlineReplace(line);

            if (isdigit( (uint8_t) line[0])) // In questo caso ignoro i punteggi
                continue;

            // Ricerco linearmente tutta la lista dei topic fino a trovare quello corrispondente
            for (int i = 0; i < context->nTopics; i++)
                if (!strcmp(line, context->topics[i].name ))
                {
                    unplayed[i] = false;
                    break;
                }

        }
        free(line);
        fclose(file);
    }
    
    context->directory[endline] = '\0'; // Resetto la context->directory

    return unplayed;
}

/* topicsPlayed
 * Per ciascuna riga non vuota:
 *   se non è un numero la considero un topic e se presente la segno come giocato
 *   se è un numero la considero il punteggio dell'ultimo topic giocato (nella riga precedente)
 * 
 * - i topic giocati senza aver trovato il punteggio avranno punteggio 0
 */
int *topicsPlayed(TopicsContext *context, char *user)
{
    int endline = strlen(context->directory);
    
    char *path = context->directory;
    strcat(path, USERS_DIR);

    // Rendo il nome minuscolo
    char tmp[CLIENT_NAME_MAX + 1];
    strncpy(tmp, user, sizeof(tmp));
    stringLower(tmp);
    strcat(path, tmp);

    strcat(path, ".txt");

    int *played = (int *) malloc(sizeof(int) * context->nTopics);
    for (int i = 0; i < context->nTopics; i++)
        played[i] = -1;

    FILE *file;
    if ((file = fopen(path, "r")))
    {
        int last = -1; // Ricorda l'ultimo indice, -1 indica non presente
        char *line = NULL;
        size_t alloc_len = 0;
        while(getline(&line, &alloc_len, file) != -1)
        {
            if ((line[0] == '\n' || // Linee vuote
                (line[0] == '\r' && line[1] == '\n'))) // caso Windows, valutazione in corto circuito
            {
                last = -1;
                continue;
            }

            newlineReplace(line);

            if (last != -1 && played[last] == 0 && isdigit( (uint8_t) line[0])) // Punteggi
            {
                if (sscanf(line, "%d", &played[last]) > 0)
                {
                    if (played[last] < 0) // non esistono punteggi minori di 0
                        played[last] = 0;
                    last = -1;
                }
                continue;
            }

            // Scansiono tutta la lista dei topic fino a trovare quello corrispondente
            for (int i = 0; i < context->nTopics; i++)
                if (!strcmp(line, context->topics[i].name ))
                {
                    last = i;
                    played[i] = false;
                    break; // break internal loop
                }

        }
        free(line);
        fclose(file);
    }
    
    context->directory[endline] = '\0'; // ripristino il percorso originale

    return played;
}

/* user.txt
 *
 * Topic 1
 * 5
 * 
 * Topic 2
 * Topic 3
 * 3
 */
bool topicMakePlayed(TopicsContext *context, char *user, int i_topic, int score)
{
    if (i_topic >= context->nTopics)
        return false;

    int endline = strlen(context->directory);
    
    char *path = context->directory;
    strcat(path, USERS_DIR);

    // Rendo il nome minuscolo
    char tmp[CLIENT_NAME_MAX + 1];
    strncpy(tmp, user, sizeof(tmp));
    stringLower(tmp);
    strcat(path, tmp);

    strcat(path, ".txt");

    FILE *file;
    if (!(file = fopen(path, "a")))
        return false;

    if (score == -1)
        fprintf(file, "%s\n", context->topics[i_topic].name);  // Se non è un punteggio scrivo il nome del giocatore
    else
        fprintf(file, "%d\n\n", score); // Altrimenti scrivo il puneggio

    fclose(file);

    context->directory[endline] = '\0';
    return true;
}

void topic_list_print_question(void * data)
{
    Question * tmp = (Question *) data;
    printf("_%s_: _%s_\n", tmp->question, tmp->answer);
}

int topics_compare(const void *a, const void *b){
    Topic * topic_a = (Topic *) a,
     * topic_b = (Topic *) b;
    return strcmp(topic_a->name, topic_b->name);
}
