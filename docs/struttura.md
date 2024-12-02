# Struttura

Strutture dati principali:
- Array dei clients (fd_sets, sockets, pipes, stato)
- Topics
- Classifica

## classifica

Potrebbe essere una lista, ma può essere un array lungo MAX_CLIENTS, si può generare dal punteggio degli array dei clients

Nel suddetto caso:
- char * classifica(struct * clients) // Ritorna il puntatore a una stringa allocata dinamicamente contenente il testo della classifica

Oppure inviare la struct ai clients in modalità binaria

## clients

Potrebbe essere una lista di struct o un array di puntatori a struct.

Con I/O Multiplexing contiene
- Se client valido
- Il Timeout
- Il punteggio
- Lo stato (indice del topic e della domanda attuale)

Concorrente (fork):
- pipes

## topics

I topic vengono memorizzati nel seguente modo:
- Un file per ogni topic, dove il nome del file è il nome del topic
- Riga domanda e la riga successiva la risposta
- Le righe bianche vengono ignorate in maniera da poterle mettere in maniera da migliorare la leggibilità

Una volta che i topics sono caricati in memoria non vengono più modificati, può essere un array dinamico

Funzioni:
- bool loadTopics(struct Topic **) // Carica tutti i topics
- loadTopic(struct Topic *, int i) // Carica il topic i-esimo, funzione di supporto
- void freeTopics() // Libera lo spazio preso dai topics 

## send and recv

// Command è un enumerato

bool sendCommand(int socket, Command command); // Invia il comando (in binario?)
bool sendMessage(int socket, char * message); // Invia il messaggio specificando prima la dimensione
int recvCommand(int socket); // Riceve un comando
char * recvMessage(int socket); // Riceve un messaggio allocato dinamicamente