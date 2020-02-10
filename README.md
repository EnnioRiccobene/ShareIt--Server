# ShareIt! - Server

ShareIt! è nato come progetto universitario e questa repository contiene il Server del progetto

ShareIt! è un sistema di editing testuale cooperativo che consenta a uno o più utenti di modificare il contenuto di un documento in contemporanea, garantendo che operazioni di inserimento o modifica diverse, svolte dagli utenti allo stesso tempo, producano gli stessi effetti, indipendentemente dall’ordine con cui sono eseguite sui diversi sistemi in uso (commutatività) e che cancellazioni ripetute portino allo stesso risultato (idempotenza).

# Requisiti del sistema
1. Architettura. Il sistema è composto da due moduli indipendenti, il client ed il server.

2. Il server è costituito da un processo costantemente attivo, in grado di accettare,
attraverso la rete, connessioni provenienti dai client. Il server, al suo interno, mantiene
un insieme di documenti che possono essere editati, in modo collaborativo dai client.
Tali documenti sono archiviati sul file system del server, così da non perderne il
contenuto in caso di accidentale interruzione del processo. Le operazioni di salvataggio
sono eseguite automaticamente e non richiedono nessuna richiesta esplicita da parte
dei client.

3. I client sono processi discontinui (possono essere avviati e terminati in modo
indipendente, secondo la volontà dell’utente), eseguiti all’interno di elaboratori
connessi in rete con il server. I client offrono un’interfaccia grafica mediante la quale
un utente può richiedere al server di editare uno dei documenti attivi o chiedere di
crearne uno nuovo, cui assegna un nome univoco. Quando il documento richiesto viene
aperto, il client offre una tipica interfaccia da editor che permette di modificare il
documento. Se due o più client modificano contemporaneamente il documento, il
server garantisce che le operazioni effettuate generino, in ciascun client una
rappresentazione coerente. I singoli client mostrano il numero e l’identità degli utenti
che stanno modificando il documento corrente ed evidenziano la posizione del cursore
dei diversi utenti all’interno del documento. Il client può evidenziare il testo introdotto
dai diversi utenti utilizzando colori differenti.

4. Gestione dell’identità. All’atto dell’avvio di un client, l’utente deve identificarsi presso il
server utilizzando opportune credenziali (username, password). Per ogni utente, il
server mantiene un profilo (nickname, icona, …) che può essere modificato dall’utente
attraverso il proprio client.

# Pubblicazione di un documento
Il client può produrre una versione PDF del documento corrente e salvarla nel proprio file
system.

# Arricchimento delle azioni
L’editor permette di copiare e incollare contenuti testuali presenti all’interno dell’elaboratore
del client. Le operazioni di modifica del testo possono essere attivate tramite shortcut da
tastiera (es.: Ctrl-C/Ctrl-V) o da opportuni menu grafici.

# Invito a collaborare
Un client può inviare ai propri conoscenti, attraverso il canale che ritiene opportuno (e-mail,
chat, …), una URI riferita al documento corrente. Copiando e incollando tale URI nel proprio
client, il destinatario dell’invito può accedere al documento e partecipare alla sua modifica.
