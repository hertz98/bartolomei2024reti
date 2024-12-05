# Tipologia del server

## Iterativo

Non sono sicuro sia nemmeno realizzabile

## Concorrente (fork())
Vantaggi:
- Utilizzo di socket bloccanti, ideologicamente più semplice gestire client per client
- Ogni processo ha la sua vita, non è necessario memorizzare lo stato

Svantaggi:
- Comunicazione tra processi difficoltosa, si devono usare metodi appositi come le pipes (ogni fork un paio di pipe da gestire)
- Le strutture dati si duplicano per ogni client (come ad esempio le domande e la classifica che rimane non aggiornata) 

In particolare:
- Una volta che il client risponde, il processo deve aggiornare la classifica sul server
- Se il client chiede la classifica, il processo servitore deve chiedere al padre la classifica aggiornata
- Il padre deve gestire il socket listener e ascoltare le pipes aperte con i figli

## Concorrente (pthreads)
Vantaggi:
- Utilizzo di socket bloccanti, ideologicamente più semplice gestire client per client
- I threads condividono hanno accesso alle stesse strutture dati
- Ogni processo ha la sua vita, non è necessario memorizzare lo stato

Svantaggi:
- Necessità di garantire la mutua esclusione alle strutture dati


### Struttura

Socket bloccanti, con timeout

- Il main thread si occupa solo di gestire le nuove richieste da parte dei client e di gestire il terminale del server
- Ogni richiesta viene affidata a un nuovo thread, che tramite socket bloccanti con timmeout si occuparà di gestire i timeout

Le strutture dati del client possono essere mantenute nella stack del metodo del thread, tranne quelle che vanno condivise
Il main thread può accedere alle strutture dati dei client tramite mutua esclusione 

## I/O Multiplexing
Vantaggi:
- Un solo thread gestisce tutto, ciclo a eventi, tutto è sincrono all'interno del server

Svantaggi:
- Socket non bloccanti, più complesso gestire client per client
- Un solo thread deve gestire tutti i client, per ciascuno di essi devo memorizzare lo stato, difficoltoso con metodi lunghi (che non ci sono)

### Struttura

Socket non bloccanti, select con timeout

- Nel main si gestiscono le nuove richieste e messaggi tramite select, i nuovi client vengono inseriti nel master set.
- Ogni client nel master set avrà la sua struttura dati nell'array dei client (che può essere istanziato nella stack)
- Allo scadere del timeout della select, si dovranno gestire il terminale e il timeout dei clients