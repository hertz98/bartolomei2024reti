# Tipologia del server

## Iterativo


## Concorrente (fork())
Vantaggi:
- Utilizzo di socket bloccanti, ideologicamente più semplice gestire client per client
 
Svantaggi:
- Comunicazione tra processi difficoltosa, vanno usate le pipes

## Concorrente (pthreads)
Vantaggi:
- Utilizzo di socket bloccanti, ideologicamente più semplice gestire client per client
- I threads condividono hanno accesso alle stesse strutture dati

Svantaggi:
- Necessità di garantire la mutua esclusione alle strutture dati

## I/O Multiplexing
Vantaggi:
- Un solo thread gestisce tutto, ciclo a eventi, tutto è sincrono all'interno del server

Svantaggi:
- Socket non bloccanti, più complesso gestire client per client

