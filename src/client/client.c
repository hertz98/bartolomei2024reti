#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

#include "../parameters.h"
#include "server_comm.h"
#include "../shared/message.h"
#include "../shared/list.h"

/********** DEFINIZIONI **********/

typedef enum InputType {INPUT_INT, INPUT_STRING} InputType;

/********** PROTOTIPI DI FUNZIONE **********/

/// @brief Clear the terminal
void clear();

/// @brief At startup, make you choose between connecting the server or exit
/// @return True if you select to continue
bool mainMenu();

/// @brief Inizializza le strutture dati del client e tenta la connessione
/// @param argc 
/// @param argv 
/// @return true in caso di connessione al server, false in caso di insuccesso
int init(int argc, char ** argv);

/// @brief Permette all'utente di inviare il proprio nickname al server, e
/// di ritentare in caso non sia valido
/// @return true in caso di successo, false in caso di errore
bool signup();

/// @brief Chiede al server le informazioni sui topic, inoltre preleva anche quelli giocabili
/// @return true in caso di successo, false in caso di errore
bool getTopicsData();

/// @brief Invia al server l'indice del topic che l'utente vuole giocare
/// @return true nel caso la selezione sia avvenuta con successo, false in caso di errore
bool topicsSelection();

/// @brief In loop, chiede al server la prossima domanda e abilita la risposta dall'utente
/// @return true al termine delle domande, false in caso di errore
bool playTopic();

/// @brief Sostituisce il primo carattere di fine linea con il carattere nullo
/// @param string 
/// @return Il puntatore al carattere successivo a quello sostituito
char * newlineReplace(char * string);

/// @brief Aspetta che l'utente prema il tasto ENTER dopo aver stampato un messaggio
void readUser_Enter();

/// @brief Chiede al server la classifica attuale
/// @return true in caso di successo, false in caso di errore o di errore di comunicazione
bool scoreboard(char * msg, ...);

/// @brief Funzione generale di input, legge quello che l'utente scrive ed
/// eventualmente interpreta i comandi "show score" e "endquiz", inoltre
/// verifica che la connessione con il server non sia caduta
/// @param type Permette di specificare se copiare un intero o una stringa nel buffer dato
/// @param buffer Solitamente un array di char o un puntatore a intero
/// @param size Dimensione dell'array di caratteri, non importa con l
/// @param server Se true controlla che la connessione col server non cada
/// @param showscore Se true permette di chiedere tramite "show score" la classifica al server e di stamparla
/// @param endquiz Se true permette di attuare il comando "endquiz" di terminare
/// @param timeout NULL per infinito
/// @return Il numero di byte copiati oppure 0 se non è stato immesso niente, 
/// oppure -1 in caso di altre operazioni oppure termina il server
int input(InputType type, void * buffer, int size, bool server, bool showscore, bool endquiz, struct timeval *timeout);

/// @brief Da internet... permette fflush(stdin)...
void clear_stdin();

/// @brief Converte l'indice del topic dal punto di vista dell'utente a quello
/// dal punto di vista dell'array dei topic nel server
/// @return Indice dello stesso topic corrispondente nell'array dei topics
int client_playableIndex(int playable);

/********** VARIABILI GLOBALI **********/

int sd = -1; // Indice del socket del server
char myname[CLIENT_NAME_MAX + 2] = "\0"; // Un byte per rilevare che il nome inserito è troppo lungo, e un'altro per il carattere di terminazione
int nTopics = 0;
char ** topics = NULL; // Contiene i nomi dei topics
bool * playableTopics = NULL;
int playing = -1;

/********** METODI **********/

int main (int argc, char ** argv)
{
    if (!mainMenu())
        return 0;
    
    if (!init(argc, argv)) // Connessione
        return 1;

    if (!signup())
        return 2;

    if (!getTopicsData())
        return 3;

    while(true) // Seleziona il topic e giocalo fino a errore o al termine
    {
        if (!topicsSelection() || !playTopic())
            break;
    }

    close(sd);

    return 4; // Se sono arrivato qua c'è stato un errore
}

int init(int argc, char ** argv)
{
    struct sockaddr_in server_addr;
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
    {
        perror("Errore nella creazione del socket:");
        return false;
    }
    
    memset(&server_addr, 0, sizeof(server_addr)); // Pulizia
    server_addr.sin_family = AF_INET;

    char * addr;
    switch (argc) // In base al numero di argomenti passati
    {
    case 0:
    case 1:
        addr = DEFAULT_BIND_IP;
        server_addr.sin_port = htons( DEFAULT_BIND_PORT );
        break;
    case 2:
        addr = DEFAULT_BIND_IP;
        server_addr.sin_port = htons( atoi( argv[1] ));
        break;
    case 3:
    default:
        addr = argv[1];
        server_addr.sin_port = htons( atoi( argv[2] ));
        break;
    }

    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) == -1)
    {
        printf("Errore nella conversione dell'indirizzo ip\n");
        return false;
    }

    if (connect(sd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    {
        perror("Errore nella connect: ");
        return false;
    }

    switch (recvCommand(sd))
    {
    case CMD_OK:
        return true;
    
    case CMD_FULL:
        printf("Il server è pieno!\n");
        return false;
    
    default:
        printf("Il server non può gestire la richiesta\n");
        return false;
    } 

    return false;
}

bool mainMenu()
{
    clear();
    printf("Trivia Quiz\n+++++++++++++++++++++++++++++++\nMenù:\n");
    printf("1 - Comincia una sessione di Trivia\n");
    printf("2 - Esci\n");
    printf("+++++++++++++++++++++++++++++++\n");

    while(true)
    {
        printf("La tua scelta: ");
        fflush(stdout);

        int selection, ret;
        if ((ret = input(INPUT_INT, &selection, sizeof(selection), false, false, false, NULL)) <= 0)
            continue;

        if (selection == 1)
            return true;
        else if (selection == 2)
            return false;
    }

}

inline void clear()
{
    printf("\033[H\033[J"); // or system("clear");
}

bool signup()
{
    int ret;

    clear();
    printf("Trivia Quiz\n+++++++++++++++++++++++++++++++\n");

    while(true)
    {
        printf("Scegli un nickname (deve essere univoco): ");
        fflush(stdout);

        if ((ret = input(INPUT_STRING, myname, sizeof(myname), true, false, false, NULL)) <= 0)
            continue;

        if (strlen(myname) < CLIENT_NAME_MIN)
        {
            printf("Troppo corto!\n");
            continue;
        }

        if (strlen(myname) > CLIENT_NAME_MAX)
        {
            printf("Troppo lungo!\n");
            continue;
        }

        if (!sendCommand(sd, CMD_REGISTER))  // Inzio la richiesta di registrazione
        {
            printf("Errore nella comunicazione...\n");
            return false;
        }

        if ( recvCommand(sd) != CMD_OK) // Aspetto conferma
        {
            printf("Rifiutato dal server...\n");
            return false;
        }

        MessageArray *tmp = messageArray(1);
        messageString(&tmp->messages[0], myname, false);
        sendMessage(sd, tmp);
        messageArrayDestroy(&tmp);

        switch(recvCommand(sd))
        {
            case CMD_OK:
                return true;
            
            case CMD_EXISTING:
                printf("Nickname duplicato!\n\n");
                continue;

            case CMD_NOTVALID:
                printf("Nickname non valido\n\n");
                continue;

            case false:
                printf("Rifiutato dal server\n");
                return false;

            default:
                printf("Errore nella comunicazione\n");
                return false;
        }
    }
    return false;
}

bool getTopicsData()
{
    if (sendCommand(sd, CMD_TOPICS) && recvCommand(sd) != CMD_OK) // Chiedo e aspetto conferma
    {
        printf("Errore nello scaricamento dei topics\n");
        return false;
    }

    if (topics) // Se sono già presenti mi tocca deallocare tutto
    {
        for (int i = 0; i < nTopics; i++)
            free(topics[i]);
        free(topics);
    }

    MessageArray *tmp = recvMessage(sd);
    if (!tmp)
    {
        printf("Errore nella ricezione dei dati sui topics dal server\n");
        return false;
    }

    nTopics = tmp->size - 1; // L'ultimo messaggio (il penultimo in realtà) contiene anche i topics giocabili
    topics = messageArray2StringArray(tmp); // Automaticamente le stringhe non vengono più deallocate
    playableTopics = (bool*) tmp->messages[nTopics].payload;
    messageArrayDestroy(&tmp);

    if (!nTopics)
    {
        printf("Nessun quiz disponibile nel server\n");
        readUser_Enter();
        exit(EXIT_SUCCESS);
    }

    return true;
}

int client_playableIndex(int index)
{
    if (index < 0 || index >= nTopics)
        return -1;
    
    for (int i = 0, n = 0; i < nTopics; i++)
        if (playableTopics[i])
            if (n++ == index)
                    return i;

    return -1;
}

bool recvPlayables()
{
    if (!(sendCommand(sd, CMD_TOPICS_PLAYABLE) && recvCommand(sd) == CMD_OK))
    {
        printf("Errore nella comunicazione");
        return false;
    }

    if (playableTopics)
    {
        free(playableTopics);
        playableTopics = NULL;
    }

    MessageArray * tmp = recvMessage(sd);
    if (!tmp)
    {
        printf("Errore di comunicazione con il server\n");
        return false;
    }

    playableTopics = tmp->messages[0].payload;

    return true;
}

bool topicsSelection()
{
    if (!playableTopics && !recvPlayables())
    {
        printf("Problema di comunicazione con il server\n");
        return false;
    }

    int nPlayable = 0;
    for (int i = 0; i < nTopics; i++)
        if (playableTopics[i])
            nPlayable++;

    while (!nPlayable) // while (true)
    {
        if (!scoreboard("Nessun quiz disponibile per l'utente \"%s\"\nPremere un ENTER per continuare\n\n", myname))
            return false;

        exit(0);
    }
    
    while(true)
    {
        int ret;

        clear();

        printf("Quiz disponibili\n+++++++++++++++++++++++++++++++\n");

        for (int i = 0, n = 0; i < nTopics; i++)
            if (playableTopics[i])
                printf("%d - %s\n", ++n, (char*) topics[i]);

        printf("+++++++++++++++++++++++++++++++\n");

        while (true)
        {
            printf("La tua scelta: ");
            fflush(stdout);
            ret = input(INPUT_INT, &playing, sizeof(playing), true, true, true, NULL);

            if (!ret)
                continue;
            else if (ret < 0)
                break; // Ristampo tutto            
            else if (playing >= 1 && playing <= nPlayable)
                break;
        }

        if (ret > 0) // altrimenti ri-stampo tutto
            break;
    }

    if (!(sendCommand(sd, CMD_SELECT) && recvCommand(sd) == CMD_OK))
    {
        printf("Errore nella comunicazione");
        return false;
    }

    playing = client_playableIndex(playing - 1);

    playableTopics[playing] = false;

    MessageArray * message_sel = messageArray(1);
    messageInteger(&message_sel->messages[0], playing);
    sendMessage(sd, message_sel);

    messageArrayDestroy(&message_sel);

    return true;
}

char * newlineReplace(char * string)
{
    for( ; *string != '\0'; string++)
    {
        if(*string == '\n')
        {
            *string = '\0';
            return string + 1;
        }
    }
    return NULL;
}

void readUser_Enter()
{
    printf("Premere [Invio] per continuare...\n");
    while (input(INPUT_STRING, NULL, 0, true, false, false, &(struct timeval) {1, 0}) < 0);
}

bool scoreboard(char * msg, ...)
{
    do
    {
        clear();
        
        if (msg) // Passo a printf i parametri passati a questa stessa funzione
        {
            va_list args;
            va_start(args, msg);    
            printf(msg, args);
            va_end(args);
        }

        if (!sendCommand(sd, CMD_SCOREBOARD) || recvCommand(sd) != CMD_OK) // Chiedo e aspetto conferma
        {
            printf("Errore di comunicazione con il server\n");
            return false;
        }

        MessageArray * tmp = recvMessage(sd);
        if (!tmp)
        {
            printf("Qualcosa è andato storto nella ricezione della classifica\n");
            return false;
        }

        for (int i = 0; i < tmp->size; i++)
            printf("%s\n", (char *) tmp->messages[i].payload);

        messageArrayDestroy(&tmp);

    } while (input(INPUT_STRING, NULL, 0, true, false, false, &(struct timeval) {1, 0}) < 0);

    return true;
}

bool playTopic()
{
    while(true) // Exyernal topic playing loop
    {
        if (!sendCommand(sd, CMD_NEXTQUESTION))
        {
            printf("Erroe di comunicazione");
            exit(0);
        }
        
        switch (recvCommand(sd))
        {
        case CMD_OK:
            break;

        case CMD_NONE:
            return true;
        
        default:
            printf("Errore di comunicazione con il server\n");
            exit(EXIT_FAILURE);
        }

        MessageArray *question_msg = recvMessage(sd);
        if (!question_msg)
        {
            printf("Errore di comunicazione con il server\n");
            return false;
        }

        question_msg->messages[0].toFree = true;

        char buffer[CLIENT_MAX_MESSAGE_LENGHT];
        while(true) // Printing loop
        {
            clear();
            printf("Quiz - %s\n+++++++++++++++++++++++++++++++\n",
                (char*) topics[playing]);

            printf("%s\n", (char *) question_msg->messages[0].payload);
            printf("\n");

            int ret;
            do
            {
                printf("Risposta: ");
                fflush(stdout);
            } while ((ret = input(INPUT_STRING, buffer, sizeof(buffer), true, true, true, NULL)) == 0);

            if (ret > 0)
                break;
            else if (ret == -2)
                return true;
        }

        if ( !sendCommand(sd, CMD_ANSWER) )
        {
            printf("Errore nell'invio della risposta\n");
            exit(EXIT_FAILURE);
        }

        MessageArray *answer_msg = messageArray(1);
        messageString(&answer_msg->messages[0], buffer, false);
        if (!sendMessage(sd, answer_msg))
        {
            printf("Errore nell'invio della risposta\n");
            return false;
        }

        messageArrayDestroy(&question_msg);
        messageArrayDestroy(&answer_msg);

        switch (recvCommand(sd))
        {
        case CMD_CORRECT:
            printf("Risposta corretta\n");
            break;

        case CMD_WRONG:
            printf("Rispsota errata\n");
            break;
        
        default:
            printf("Errore nella ricezione del verdetto\n");
            return false;
            break;
        }

    readUser_Enter();
    }
}

int input(InputType type, void * out_buffer, int size, bool server, bool showscore, bool endquiz, struct timeval *timeout)
{
    char buffer[CLIENT_MAX_MESSAGE_LENGHT];
    int ret;

    if (size < 0)
        size = 0;

    // Uso la select per controllare sia STDIN che il socket del server in maniera di accorgermi della disconnessione
    while(server)
    {
        int ready = client_socketsReady( (int[]) {STDIN_FILENO, sd, }, 2, timeout);
        if (ready == -1)
            return -1;

        if (ready == sd)
            if (recvCommand(sd) == false)
            {
                printf("Connessione con il server interrotta\n");
                exit(EXIT_FAILURE);
            }
        
        if (ready == STDIN_FILENO)
            break;
        
    }

    // Lettura di STDIN
    if ((ret = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[ret - 1] = '\0';

        if (!strncmp(buffer, "quit", sizeof(buffer)) ||
                !strncmp(buffer, "exit", sizeof(buffer)) )
            exit(EXIT_SUCCESS);

        if (showscore && !strncmp(buffer, "show score", sizeof(buffer)))
        {
            if (!scoreboard(NULL))
                return false;
            return -1;
        }

        if (endquiz && !strncmp(buffer, "endquiz", sizeof(buffer)))
        {
            sendCommand(sd, CMD_ENDQUIZ);
            return -2;
        }

        if (size)
        {
            if (type == INPUT_STRING)
            {
                strncpy(out_buffer, buffer, size - 1);
                buffer[size - 1] = '\0';
                return ret - 1;
            }
    
            if (type == INPUT_INT && (ret = sscanf(buffer, "%d", (int*) out_buffer)) > 0)
                return ret;
            else
                return 0;
        }
        else
            return ret - 1;
    }
    else if (ret == 0)
    {
        printf("EOF rilevato, uscita...\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        perror("Errore: ");
        exit(EXIT_FAILURE);
    }
}

void clear_stdin() {
    char ch;
    while ((ch = getchar()) != '\n' && ch != EOF)  // Read and discard characters until newline or EOF
        ;
}