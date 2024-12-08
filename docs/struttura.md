# Struttura

Questo documento a l'utilità di descrivere sinteticamente l'applicazione client server e i suoi metodi

Strutture dati principali:
- Array dei clients (fd_sets, sockets, pipes, stato)
- Topics
- Classifica

## classifica

Potrebbe essere una lista, ma può essere un array lungo MAX_CLIENTS, si può generare dal punteggio degli array dei clients

Nel suddetto caso:
- char * ranking(struct * clients) // Ritorna il puntatore a una stringa allocata dinamicamente contenente il testo della classifica

Oppure inviare la struct ai clients in modalità binaria

## clients

Potrebbe essere una lista di struct o un array di puntatori a struct Client, sempre allocati dinamicamente.

Con I/O Multiplexing contiene
- Se client valido
- Il timestamp
- Il punteggio
- Lo stato (indice del topic e della domanda attuale oppure indice del topic e puntatore al topic corrente)

Concorrente (fork):
- pipes

### timeout 

Alla creazione del socket viene inizializzato ad un valore predefinito ed a ogni intervallo di tempo costante si decrementa, se raggiunge zero il client/server è disconnesso

A periodi di tempo costanti si inviano messaggi per sapere se il client/server è ancora attivo/presente.

## topics

I topic vengono memorizzati nel seguente modo:
- Un file per ogni topic, dove il nome del file è il nome del topic
- Riga domanda e la riga successiva la risposta
- Le righe bianche vengono ignorate in maniera da poterle mettere in maniera da migliorare la leggibilità

Una volta che i topics sono caricati in memoria non vengono più modificati, può essere un array dinamico

Funzioni:
- bool topicsLoader(struct Topic **) // Carica tutti i topics
- topicLoad(int i, struct Topic *) // Carica il topic i-esimo, funzione di supporto
- void topicsFree() // Libera lo spazio preso dai topics

Se si usano gli array:
- struct Topic * topicQuestion(int t, int i) // Ritorna la stringa della i-esima domanda del topic t-esimo, null se non esiste

```
void topicLoad(int i, struct Topic *)
{
    // Scorro il file e mi ricavo il numero di domande/risposte valide
    // creo un buffer di dimensione QUESTION_MAX_SIZE che utilizzerò come locazione temporanea per la lettura del file
    // Creo lo array e riapro il file
    foreach(riga){
        // scrivo la domanda/riposta nel buffer
        // malloc e la aggiungo alla struttura
        // se struttura completa, la aggiungo all'array o alla lista
    }
}
```

## send and recv

// Command è un enumerato o un array di stringhe

bool sendCommand(int socket, Command command); // Invia il comando (in binario?)
bool sendMessage(int socket, char * message); // Invia il messaggio specificando prima la dimensione
Enum Command recvCommand(int socket); // Riceve un comando
char * recvMessage(int socket); // Riceve un messaggio allocato dinamicamente, deve essere deallocato

Ogni qual volta si riceve il dato da un client, il suo timeout viene resettato.

## chiusura

La comunicazione può essere chiusa tramite comando normalmente dal client o dal server, per timeout o qualsiasi ragione.

In caso di chiusura del server tramite segnale del sistema operativo, si affida la chiusira dei socket a una funzione specifica (atexit e signal) ragione per cui i socket sono globali.
