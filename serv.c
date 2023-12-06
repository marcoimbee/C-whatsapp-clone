// -------------------- INCLUDES --------------------
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>



// -------------------- DEFINITIONS --------------------
#define DISCONNECTION_ACKS 3            //ACKs di disconnessione inviati al momento della disconnessione del server ad ogni utente connesso
#define CREDENTIALS_LENGTH 20
#define TIMESTAMP_LENGTH 20 
#define BUFFER_SIZE 1024
#define ANSI_COLOR_GREEN "\x1b[32m"     //colore verde in console per i messaggi di stato (verbose)
#define ANSI_COLOR_RED "\x1b[31m"     //colore rosso in console per i messaggi di stato (verbose)
#define ANSI_COLOR_RESET "\x1b[0m"      //reset per tornare al colore originale
#define TRUE 1
#define FALSE 0



// -------------------- GLOBAL STRUCTURES AND VARIABLES --------------------
struct users_data{
    char username[CREDENTIALS_LENGTH];
    char password[CREDENTIALS_LENGTH];
    int port;
    char timestamp_login[TIMESTAMP_LENGTH];
    char timestamp_logout[TIMESTAMP_LENGTH];
};

struct credentials_entry{
    char username[CREDENTIALS_LENGTH];
    char password[CREDENTIALS_LENGTH];
};

struct register_entry{
    char username[CREDENTIALS_LENGTH];
    int port;
    char timestamp_login[TIMESTAMP_LENGTH];
    char timestamp_logout[TIMESTAMP_LENGTH];
};

struct users_operation{
    char command[BUFFER_SIZE];
    char parameter[BUFFER_SIZE];
};

struct chat_log_entry{
    char sender[CREDENTIALS_LENGTH];
    char recipient[CREDENTIALS_LENGTH];
    char msg[BUFFER_SIZE];
    uint8_t delivered;
    char timestamp[TIMESTAMP_LENGTH];
};

struct pending_log_entry{
    char from[CREDENTIALS_LENGTH];
    char to[CREDENTIALS_LENGTH];
    int tot_msg;
    char most_recent_timestamp[TIMESTAMP_LENGTH];
};



// -------------------- FUNCTIONS AND METHODS --------------------
/*
    void list_command()

    La funzione mostra a video numero e (se > 0)
    username, timestamp di login e porta degli utenti online che sono
    connessi al server.
*/
void list_command(){
    FILE *server_register;
    int counter = 0;
    struct register_entry register_read;
    char exit_list;

    server_register = fopen("SERVER_FILES/server_register.bin", "rb");
    if(server_register == NULL){        //se il file è vuoto nessun utente è online al momento
        printf(ANSI_COLOR_GREEN "> Nessun utente online al momento.\n" ANSI_COLOR_RESET);
        printf("\n");
        fclose(server_register);
        return;
    }

    while(fread(&register_read, sizeof(struct register_entry), 1, server_register) == 1){   //si contano gli utenti online
        if(strlen(register_read.timestamp_logout) == 0){
            counter++;
        }
    }

    rewind(server_register);

    if(counter == 0){
        printf(ANSI_COLOR_GREEN "> Nessun utente online al momento.\n" ANSI_COLOR_RESET);
    }else{
        printf(ANSI_COLOR_GREEN "> %d utente/i online al momento:\n" ANSI_COLOR_RESET, counter);      //stampa dei dati a video
        while(fread(&register_read, sizeof(struct register_entry), 1, server_register) == 1){
            if(strlen(register_read.timestamp_logout) == 0){
                printf("    > %s*%s*%d\n", register_read.username, register_read.timestamp_login, register_read.port);
            }
        }
    }

    fclose(server_register);

    printf("\n");
}


/*
    void display_menu()

    Parametri:
        -> uint16_t server_port: porta da mostrare nella stampa a video

    La funzione stampa a video il menu dei comandi accessibili dal server.
*/
void display_menu(uint16_t server_port){
    printf("\n");
    printf(
        "******************** SERVER STARTED (listening on port %d) ********************\n", server_port);
    printf("Digitare un comando:\n");
    printf("\n");
    printf("1) help --> mostra una guida dei comandi disponibili\n");
    printf("2) list --> mostra un elenco degli utenti connessi al momento\n");
    printf("3) esc  --> chiude il server\n");
}


/*
    void help_command()

    La funzione mostra una breve guida dei comandi accessibili dal server.
*/
void help_command(){
    printf("\n");
    printf(ANSI_COLOR_GREEN "********** Breve descrizione dei comandi **********\n" ANSI_COLOR_RESET);
    printf("1) help --> mostra a video una breve descrizione dei comandi possibili.\n");
    printf("2) list --> mostra a video l'elenco degli utenti connessi alla rete,\n");
    printf("            indicando username, timestamp di connessione e numero di porta\n");
    printf("            su cui sono connessi nel formato 'username*timestamp*porta'.\n");
    printf("3) esc  --> termina il server, non impedendo però alle chat in corso di proseguire.\n");
    printf("\n");
}


/*
    void register_server_data()

    Parametri:
        -> struct sockaddr_in server_address: struct dei dati del server da cui estrarre la porta da registerare su file

    La funzione registra la porta su cui è attivo il server sul file 'SERVER_FILES/server_data.bin'.
*/
void register_server_data(struct sockaddr_in server_address){
    FILE *server_data;
    uint16_t port_local;

    server_data = fopen("SERVER_FILES/server_data.bin", "wb");
    if(server_data == NULL){
        perror("Errore nella creazione del file dei dati del server \n");
        exit(-1);
    }

    port_local = ntohs(server_address.sin_port);
    fwrite(&port_local, sizeof(uint16_t), 1, server_data);
    
    if(fwrite != 0){
        //printf(ANSI_COLOR_GREEN "> Dati del server registrati sul file 'server_data.bin'.\n" ANSI_COLOR_RESET);
    }else{
        printf("Errore nella scrittura del file.\n");
        exit(-1);
    }

    fclose(server_data);
}


/*
    uint8_t execuite_signup()

    Parametri:
        -> struct users_data users_input: dati dell'utente appena registrato da registrare sul file delle credenziali

    Controllo della validità dell'username scelto (se è o no già stato scelto).
    Registrazione dell'associazione username_password su file 'credenziali.bin'.
*/
uint8_t execute_signup(struct users_data users_input){
    //return 1: successo, username e password registrati
    //return 0: username già scelto, insuccesso
    struct credentials_entry credentials_local;          //struct locale da scrivere sul file delle associazioni username-password
    struct credentials_entry credentials_read;              //struct dove inserire il record letto dal file per controllo
    FILE *credentials;

    credentials = fopen("SERVER_FILES/credentials.bin", "ab+");        //file credenziali in modalità append e read in formato binario

    while(fread(&credentials_read, sizeof(struct credentials_entry), 1, credentials) == 1){
        if(strcmp(credentials_read.username, users_input.username) == 0){
            fclose(credentials);        //chiusura file
            return 0;                   //return codice di insuccesso
        }
    }

    //APPEND SUL FILE DELLE CREDENZIALI
    memset(&credentials_local.username, 0, sizeof(credentials_local.username));
    memset(&credentials_local.password, 0, sizeof(credentials_local.password));
    strcpy(credentials_local.username, users_input.username);
    strcpy(credentials_local.password, users_input.password);
    fwrite(&credentials_local, sizeof(struct credentials_entry), 1, credentials);

    fclose(credentials);

    return 1;           //return codice di successo
}


/*
    uint8_t execute_in()

    Parametri:
        -> struct users_data users_input: 

    autentica l'utente
    controlla se username e password sono giusti dal file credenziali_server.bin
    se i dati sono corretti setta gli altri parametri dell'utente nel file registro_server.bin
*/
uint8_t execute_in(struct users_data users_input){    
    FILE *server_register;          //file registro.bin
    FILE *credentials;              //file credenziali.bin
    FILE *temp_register;            //file registro temporaneo

    struct credentials_entry credentials_check;
    struct register_entry register_local;
    struct register_entry register_substitute;

    uint8_t loggable = 0;              //vale 1 se username e password corrispondono, 0 altrimenti (non corrispondono o l'username non esiste)
    
    time_t timestamp;                   //current timestamp
    char *timestamp_string;             //per la conversione in stringa leggibile del timestamp corrente

    credentials = fopen("SERVER_FILES/credentials.bin", "rb");

    while(fread(&credentials_check, sizeof(struct credentials_entry), 1, credentials) == 1){        //controllo la presenza o meno dell'username passato
        if(strcmp(credentials_check.username, users_input.username) == 0){          //trovato l'username, va controllata la password
            if(strcmp(credentials_check.password, users_input.password) == 0){
                loggable = 1;
                break;
            }
        }
    }

    fclose(credentials);

    if(!loggable) return 0;        //username inserito dall'utente non esiste o la password non vi corrisponde, autenticabile rimane a 0

    timestamp = time(NULL);                 //get del current timestamp e conversione a stringa leggibile dentro timestmp_string
    timestamp_string = ctime(&timestamp);

    //settaggio dei campi del record aggiornato da sostituire a quello obsoleto nel registro
    strcpy(register_substitute.username, users_input.username);         //copia username
    register_substitute.port = users_input.port;                        //copia della porta su cui si è connesso adesso il device
    strcpy(register_substitute.timestamp_login, timestamp_string);      //settaggio del timestamp_login al current_timestamp
    memset(&register_substitute.timestamp_logout, 0, sizeof(register_substitute.timestamp_logout));     //azzeramento del timestamp_logout, ora l'utente è online

    //spertura dei due file su cui su va a operare
    server_register = fopen("SERVER_FILES/server_register.bin", "ab+");             //lettura
    temp_register = fopen("SERVER_FILES/server_register_temp.bin", "ab");          //append

    //copiatura di tutti i record da registro.bin a registro_temp.bin tranne di quello da sostituire
    while(fread(&register_local, sizeof(struct register_entry), 1, server_register) == 1){
        if(strcmp(register_local.username, register_substitute.username) != 0){
            fwrite(&register_local, sizeof(struct register_entry), 1, temp_register);
        }
    }

    //scrittura del record aggiornato
    fwrite(&register_substitute, sizeof(struct register_entry), 1, temp_register);

    fclose(server_register);        //chiusura stream
    fclose(temp_register);

    remove("SERVER_FILES/server_register.bin");            //rimozione di registro.bin, ormai non aggiornato
    rename("SERVER_FILES/server_register_temp.bin", "SERVER_FILES/server_register.bin");      //registro_temp.bin, aggiornato, diventa il nuovo registro.bin 
    
    return 1;
}


/*
    setta il timestamp_logout dell'utente che si disconnette
    resetta il timestamp_login dell'utente che si disconnette
*/
void register_users_logout(struct users_data record_data){
    FILE *server_register;          //file registro.bin
    FILE *temp_register;            //file registro temporaneo

    struct register_entry register_local;
    struct register_entry register_substitute;
    
    time_t timestamp;                    //current timestamp
    char *timestamp_string;             //per la conversione in stringa leggibile del timestamp corrente
    timestamp = time(NULL);                 //get del current timestamp e conversione a stringa leggibile dentro timestmp_string
    timestamp_string = ctime(&timestamp);

    //settaggio dei campi del record aggiornato da sostituire a quello obsoleto nel registro
    strcpy(register_substitute.username, record_data.username);         //copia username
    register_substitute.port = 0;                        //porta a NULL essendo il device offline
    strcpy(register_substitute.timestamp_logout, timestamp_string);      //settaggio del timestamp_logout al current_timestamp

    //apertura dei due file su cui su va a operare
    server_register = fopen("SERVER_FILES/server_register.bin", "ab+");             //lettura
    temp_register = fopen("SERVER_FILES/server_register_temp.bin", "ab");          //append

    //copiatura di tutti i record da registro.bin a registro_temp.bin tranne di quello da sostituire
    while(fread(&register_local, sizeof(struct register_entry), 1, server_register) == 1){
        if(strcmp(register_local.username, register_substitute.username) != 0){
            fwrite(&register_local, sizeof(struct register_entry), 1, temp_register);
        }
    }

    //scrittura del record aggiornato
    fwrite(&register_substitute, sizeof(struct register_entry), 1, temp_register);

    fclose(server_register);        //chiusura stream
    fclose(temp_register);

    remove("SERVER_FILES/server_register.bin");            //rimozione di registro.bin, ormai non aggiornato
    rename("SERVER_FILES/server_register_temp.bin", "SERVER_FILES/server_register.bin");      //registro_temp.bin, aggiornato, diventa il nuovo registro.bin 
}


/*
    void manage_error()

    Parametri:
        -> int ret_val: valore di ritorno di send/recv/sendto/recvfrom usata nel programma
        -> char *descr: descrizione dell'operazione che si è eseguita
*/
void manage_error(int ret_val, char *descr){
    //printf("DESCR: %s ---> ", descr);
    if(ret_val == -1){
        perror("Error \n");
        fflush(stdin);
        exit(1);
    }else{
        //printf("Send/recv returned %d\n", ret_val);
        fflush(stdin);
    }
}


/*
    uint16_t is_online()

    Parametri:
        -> char *user_target: utente su cui si esegue il controllo

    La funzione controlla se user_target è online o offline.
    Se è online restituisce il numero di porta su cui è raggiungibile, altrimenti 0.
*/
uint16_t is_online(char *user_target){
    FILE *server_register;                  //file registro
    struct register_entry register_read;    //struct record per la lettura

    server_register = fopen("SERVER_FILES/server_register.bin", "rb");          //aprtura del registro in lettura binaria

    while(fread(&register_read, sizeof(struct register_entry), 1, server_register) == 1){       
        if(strcmp(user_target, register_read.username) == 0){           //record trovato
            if(strcmp(register_read.timestamp_logout, "") == 0){    //se il timestamp_logout è vuoto l'utente è online
                return register_read.port;           //si legge la porta a cui è raggiungibile
            }else{
                return 0;            //altrimenti è offline e si ritorna 0
            }
            break;
        }
    }

    return 0;       //non si arriva mai qui
}


/*
    uint16_t existing_username()

    Parametri:
        -> char *username: username su cui si esegue il controllo

    La funzione restituisce 1 se username è un username registrato nel sistema, 0 altrimenti.
*/
uint16_t existing_username(char *username){
    uint16_t existing = 0;
    FILE *credentials;
    struct credentials_entry credentials_read;

    credentials = fopen("SERVER_FILES/credentials.bin", "rb");

    while(fread(&credentials_read, sizeof(struct credentials_entry), 1, credentials) == 1){
        if(strcmp(credentials_read.username, username) == 0){
            existing = 1;       //username trovato, stop al while
            break;
        }
    }

    return existing;
}



// -------------------- MAIN --------------------
int main(int argc, char* argv[]){

    //SERVER PARAMETERS VARs
    int server_port;                        //porta su cui ascolta il server
    struct sockaddr_in server_address;      //indirizzo server
    const int options = TRUE;               //per setsockopt()

    //CLIENT-SERVER COMMUNICATION VARs
    struct sockaddr_in device_address;      //indirizzo client connesso
    int deviceaddr_length;                  //lunghezza indirizzo device generico
    uint8_t check;   
    int listening_socket;                   //id del socket di ascolto del server
    int comm_socket;                        //id del socket di comunicazione
    pid_t pid_menu;                         //discriminante per fork() tra listen e menu
    pid_t pid_requests;                     //gestione richieste

    //SERVER UI VARs
    char users_choice[50];                  //scelta dell'opzione del menu dell'utente
    int menu_choice;


    //inizializzazione dei parametri del server
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    if(argv[1] != NULL){
        server_port = atoi(argv[1]);        //recupero del parametro specificato all'avvio del server
    }else{
        server_port = 4242;                 //senza parametro, la porta è 4242
    }
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);      //ascolto su localhost

    //creazione file necessari
    fclose(fopen("SERVER_FILES/pending_log.bin", "ab"));
    fclose(fopen("SERVER_FILES/chat_log.bin", "ab"));
    fclose(fopen("SERVER_FILES/server_data.bin", "ab"));
    fclose(fopen("SERVER_FILES/server_register.bin", "ab"));
    fclose(fopen("SERVER_FILES/credentials.bin", "ab"));

    //registrazione dati server su file
    register_server_data(server_address);
 
    //socket di ascolto, TCP, BLOCCANTE
    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(listening_socket < 0){
        perror(ANSI_COLOR_RED "Errore nell'apertura del SOCKET " ANSI_COLOR_RESET);
        exit(-1);
    }

    //binding dei dati del server
    check = setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &options, sizeof(options));
    check = bind(listening_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if(check < 0){
        perror(ANSI_COLOR_RED "Errore in fase di BIND " ANSI_COLOR_RESET);
        exit(-1);
    }

    //inizializzazione di listening_socket come socket di ascolto, backlog = 100
    check = listen(listening_socket, 100);          
    if(check < 0){
        perror(ANSI_COLOR_RED "Errore in fase di LISTEN " ANSI_COLOR_RESET);
        exit(-1);
    }

    deviceaddr_length = sizeof(device_address);         //calcolo lunghezza indirizzo device
    

    pid_menu = fork();      //fork tra processo menu e listener
    
    //PROCESSO FIGLIO
    if(pid_menu == 0){      //processo figlio, fa la fork tra processo LISTENER e processo per il controllo di disconnessione

        while(1){
            comm_socket = accept(listening_socket, (struct sockaddr*)&device_address, &deviceaddr_length);
            setsockopt(AF_INET, SOL_SOCKET, SO_REUSEADDR, (char*)&options, sizeof(options));

            pid_requests = fork();      //ad ogni req un nuovo figlio che la serve
            if(pid_requests == 0){      //PROCESSO FIGLIO CHE SERVE LA RICHIESTA

                char command[1024];
                char msg[1024];
                uint16_t msg_len;
                int len;

                struct users_data users_data;

                close(listening_socket);        //al figlio non serve il socket di ascolto e lo chiude
                
                /*
                    In ogni req si rimane in attesa della dimensione del messaggio del device.
                    Se questa è 0 il socket è stato chiuso e si termina il processo.
                    Se si riceve una dimensione valida, si entra nell'if.
                    Si attende il messaggio serializzato dal device e lo si deserializza.

                    Se il comando fornito è 'signup', si chiama l'apposita funzione, si recapita l'esito al device
                    e si termina il processo, visto che nel device dopo la ricezione si esegue la chiusura del socket.

                    Se il comando fornito è 'in', si entra in un while(1).
                    Si chiama l'apposita funzione che esegue il login.
                */
                check = recv(comm_socket, (void*)&msg_len, sizeof(uint16_t), 0);    //ricezione della dimensione
                if(check != 0){                 //se recv riceve != 0 il socket remoto non è stato chiuso
                    len = ntohs(msg_len);

                    check = recv(comm_socket, (void*)msg, len, 0);           //ricezione della stringa di dimensione msg_len dal device
                    sscanf(msg, "%s", &command);        //deserializzazione della stringa ricevuta, prendo solo il comando

                    //SIGNUP
                    if(strcmp(command, "signup") == 0){         
                        uint8_t outcome = 0;

                        sscanf(msg, "%s %s %s", &command, &users_data.username, &users_data.password);        //deserializzazione della stringa ricevuta

                        outcome = execute_signup(users_data);       //registrazione dell'utente al servizio

                        check = send(comm_socket, (void*)&outcome, sizeof(uint8_t), 0);    //invio l'esito al device
                        if(outcome == 1) printf(ANSI_COLOR_GREEN "User '%s' registered.\n" ANSI_COLOR_GREEN, users_data.username);

                        close(comm_socket);             //servita la richiesta di signup si chiude il socket di comunicazione
                        exit(0);
                    }

                    //IN
                    if(strcmp(command, "in") == 0){                      
                        uint8_t outcome = 1;
                        
                        sscanf(msg, "%s %d %s %s", &command, &users_data.port, &users_data.username, users_data.password);        //deserializzazione della stringa ricevuta
                        outcome = execute_in(users_data);       //esegue il login dell'utente

                        check = send(comm_socket, (void*)&outcome, sizeof(uint8_t), 0);    //invio l'esito al device
                        if(outcome == 1){               //se l'esito è = 1 l'utente ha fornito i dati corretti
                            printf(ANSI_COLOR_GREEN "'%s' logged in.\n" ANSI_COLOR_RESET, users_data.username);

                            //ATTESA DI NUOVI COMANDI DA users_data.username
                            while(TRUE){
                                uint16_t operation_length;                    
                                int operation_len;
                                char operation_msg[BUFFER_SIZE];
                                struct users_operation users_operation;

                                //memset delle strutture/variabili necessarie nel ciclo
                                memset(&operation_msg, 0, sizeof(operation_msg));
                                memset(&users_operation, 0, sizeof(struct users_operation));
                                fflush(stdin);

                                check = recv(comm_socket, (void*)&operation_length, sizeof(uint16_t), 0);     //ricezione della dimensione del messaggio
                                manage_error(check, "Receiving serialized cmd and param string length");

                                //DISCONNESSIONE IMPROVVISA DI users_data.username
                                if(check == 0){
                                    printf(ANSI_COLOR_RED "User '%s' disconnected without warning.\n" ANSI_COLOR_RESET, users_data.username);
                                    
                                    register_users_logout(users_data);
                                    break;
                                }

                                operation_len = ntohs(operation_length);                                //conversione lunghezza messaggio in arrivo

                                check = recv(comm_socket, (void*)operation_msg, operation_len, 0);      //ricezione della stringa contenente comando ed eventuale parametro
                                manage_error(check, "Receiving serialized cmd and param");

                                sscanf(operation_msg, "%s", &users_operation.command);                  //deserializzazione del comando in arrivo

                                //se è un comando con un parametro, si deserializza anche quello
                                if(strcmp(users_operation.command, "show") == 0 || strcmp(users_operation.command, "chat") == 0 || strcmp(users_operation.command, "share") == 0){
                                    sscanf(operation_msg, "%s %s", &users_operation.command, &users_operation.parameter);   
                                }


                                //SHOW username
                                if(strcmp(users_operation.command, "show") == 0){
                                    uint16_t valid_target;

                                    valid_target = existing_username(users_operation.parameter);    //controlla se l'username passato esiste
                                    valid_target = htons(valid_target);
                                    check = send(comm_socket, &valid_target, sizeof(uint16_t), 0);   //invio info sul target scelto dal device
                                    manage_error(check, "Sending info about the chosen target");
                                    
                                    valid_target = ntohs(valid_target);
                                    
                                    if(valid_target){           //se il contatto scelto esiste, si procede con l'operazione, altrimenti si finisce
                                        FILE *chat_log;
                                        FILE *chat_log_temp;
                                        struct chat_log_entry chat_log_read;
                                        char send_data[BUFFER_SIZE];
                                        uint16_t send_data_length;
                                        int send_data_len;

                                        FILE *pending_log;
                                        FILE *pending_log_temp;
                                        struct pending_log_entry pending_log_read;

                                        chat_log = fopen("SERVER_FILES/chat_log.bin", "rb");
                                        chat_log_temp = fopen("SERVER_FILES/chat_log_temp.bin", "ab");

                                        //ciclo di invio dei messaggi pendenti e dei loro timestamp
                                        while(fread(&chat_log_read, sizeof(struct chat_log_entry), 1, chat_log) == 1){
                                            if(strcmp(chat_log_read.sender, users_operation.parameter) == 0 && strcmp(chat_log_read.recipient, users_data.username) == 0){                                            
                                                //invio dimensione msg e msg
                                                memset(&send_data, 0, sizeof(send_data));
                                                strcpy(send_data, chat_log_read.msg);
                                                send_data_len = strlen(send_data);
                                                send_data_length = htons(send_data_len);
                                                check = send(comm_socket, (void*)&send_data_length, sizeof(uint16_t), 0);
                                                check = send(comm_socket, (void*)send_data, send_data_len, 0);

                                                //invio dimensione timestamp e timestamp
                                                memset(&send_data, 0, sizeof(send_data));
                                                strcpy(send_data, chat_log_read.timestamp);
                                                send_data_len = strlen(send_data);
                                                send_data_length = htons(send_data_len);
                                                check = send(comm_socket, (void*)&send_data_length, sizeof(uint16_t), 0);
                                                check = send(comm_socket, (void*)send_data, send_data_len, 0);
                                            }else{
                                                //si ricopia il record
                                                fwrite(&chat_log_read, sizeof(struct chat_log_entry), 1, chat_log_temp);
                                            }
                                        }

                                        //invio dimensione ack e ack
                                        memset(&send_data, 0, sizeof(send_data));
                                        strcpy(send_data, "END_SHARE");
                                        send_data_len = strlen(send_data);
                                        send_data_length = htons(send_data_len);
                                        check = send(comm_socket, (void*)&send_data_length, sizeof(uint16_t), 0);
                                        check = send(comm_socket, (void*)send_data, send_data_len, 0);

                                        fclose(chat_log);
                                        fclose(chat_log_temp);          //update del file chat_log.bin

                                        remove("SERVER_FILES/chat_log.bin");
                                        rename("SERVER_FILES/chat_log_temp.bin", "SERVER_FILES/chat_log.bin");

                                        //aggiornamento file pending_log.bin
                                        pending_log = fopen("SERVER_FILES/pending_log.bin", "rb");
                                        pending_log_temp = fopen("SERVER_FILES/pending_log_temp.bin", "ab");

                                        while(fread(&pending_log_read, sizeof(struct pending_log_entry), 1, pending_log) == 1){
                                            if(strcmp(pending_log_read.from, users_operation.parameter) == 0 && strcmp(pending_log_read.to, users_data.username) == 0){
                                                //se trova il record non lo copia nel file aggiornato
                                            }else{
                                                fwrite(&pending_log_read, sizeof(struct pending_log_entry), 1, pending_log_temp);
                                            }
                                        }

                                        fclose(pending_log);
                                        fclose(pending_log_temp);

                                        remove("SERVER_FILES/pending_log.bin");
                                        rename("SERVER_FILES/pending_log_temp.bin", "SERVER_FILES/pending_log.bin");
                                    }
                                }

                                //HANGING
                                if(strcmp(users_operation.command, "hanging") == 0){
                                    FILE *pending_log;
                                    struct pending_log_entry pending_log_read;
                                    char send_data[BUFFER_SIZE];
                                    uint16_t send_data_length;
                                    int send_data_len;

                                    pending_log = fopen("SERVER_FILES/pending_log.bin", "rb");

                                    while(fread(&pending_log_read, sizeof(struct pending_log_entry), 1, pending_log) == 1){
                                        if(strcmp(pending_log_read.to, users_data.username) == 0){
                                        
                                            //invio di mittente e tot messaggi
                                            memset(&send_data, 0, sizeof(send_data));
                                            sprintf(send_data, "%s %d", pending_log_read.from, pending_log_read.tot_msg);
                                            send_data_len = strlen(send_data);
                                            send_data_length = htons(send_data_len);
                                            check = send(comm_socket, (void*)&send_data_length, sizeof(uint16_t), 0);
                                            check = send(comm_socket, (void*)send_data, send_data_len, 0);

                                            //invio del timestamp (contiene spazi)
                                            memset(&send_data, 0, sizeof(send_data));
                                            strcpy(send_data, pending_log_read.most_recent_timestamp);
                                            send_data_len = strlen(send_data);
                                            send_data_length = htons(send_data_len);
                                            check = send(comm_socket, (void*)&send_data_length, sizeof(uint16_t), 0);
                                            check = send(comm_socket, (void*)send_data, send_data_len, 0);
                                        }
                                    }

                                    fclose(pending_log);

                                    //invio ack di fine trasmissione
                                    memset(&send_data, 0, sizeof(send_data));
                                    strcpy(send_data, "END_PENDING");
                                    send_data_len = strlen(send_data);
                                    send_data_length = htons(send_data_len);
                                    check = send(comm_socket, (void*)&send_data_length, sizeof(uint16_t), 0);
                                    check = send(comm_socket, (void*)send_data, send_data_len, 0);
                                }
                                
                                //CHAT username
                                if(strcmp(users_operation.command, "chat") == 0){
                                    uint16_t search_outcome;      //vale: 0 se user_target è offline, port se user_target è online
                                    uint16_t valid_target;

                                    valid_target = existing_username(users_operation.parameter);    //controlla se l'username passato esiste
                                    valid_target = htons(valid_target);
                                    check = send(comm_socket, &valid_target, sizeof(uint16_t), 0);   //invio info sul target scelto dal device
                                    manage_error(check, "Sending info about the chosen target");
                                    
                                    valid_target = ntohs(valid_target);      

                                    if(valid_target){           //il parametro specificato dall'utente è valido, si controlla se è online
                                        search_outcome = is_online(users_operation.parameter);  //return 0 se user_target è offline, port se user_target è online

                                        //conversione - invio al device - controllo errori
                                        search_outcome = htons(search_outcome);
                                        check = send(comm_socket, &search_outcome, sizeof(uint16_t), 0);
                                        manage_error(check, "Sending target port to device");

                                        //se il target non è amico non si entra nel ciclo di ascolto
                                        uint8_t target_is_friend;
                                        check  = recv(comm_socket, &target_is_friend, sizeof(uint8_t), 0);
                                        manage_error(check, "Receiving if target is friend or not");

                                        if(target_is_friend){   //se il target è amico si inizia ad asoltare
                                            uint8_t recvd_command;
                                            while(TRUE){                //attesa di eventuali comandi dalla chat
                                                recvd_command = -1;

                                                char device_msg[BUFFER_SIZE];      //strutture per la memorizzazizone del msg
                                                uint16_t device_msg_length;
                                                int device_msg_len;
                                                struct chat_log_entry chat_log_entry;

                                                //prima di ogni azione del dev che coinvolga il server, si riceve il codice che la identifica
                                                check = recv(comm_socket, &recvd_command, sizeof(uint8_t), 0);
                                                manage_error(check, "Receiving chat command code from device");

                                                //DISCONNESSIONE IMPROVVISA DI users_data.username
                                                if(check == 0){
                                                    printf(ANSI_COLOR_RED "User '%s' disconnected without warning.\n" ANSI_COLOR_RESET, users_data.username);
                                                    
                                                    register_users_logout(users_data);
                                                    break;
                                                }
                                            
                                                //AZIONI DELL'UTENTE CHE COINVOLGONO IL SERVER, A SECONDA DEL CODICE RICEVUTO SI ESEGUE L'AZIONE
                                                if(recvd_command == 1){         //MESSAGGIO VERSO TARGET OFFLINE -> CODICE 1
                                                    /*
                                                        TODO: operation description
                                                    */

                                                    memset(&device_msg, 0, sizeof(device_msg));
                                                    memset(&chat_log_entry, 0, sizeof(struct chat_log_entry));

                                                    //ricezione lunghezza e mittente e settaggio campo relativo 
                                                    check = recv(comm_socket, (void*)&device_msg_length, sizeof(uint16_t), 0);
                                                    device_msg_len = ntohs(device_msg_length);
                                                    check = recv(comm_socket, (void*)device_msg, device_msg_len, 0);
                                                    strcpy(chat_log_entry.sender, device_msg);

                                                    //ricezione lunghezza e destinatario e settaggio campo relativo 
                                                    memset(&device_msg, 0, sizeof(device_msg));
                                                    check = recv(comm_socket, (void*)&device_msg_length, sizeof(uint16_t), 0);
                                                    device_msg_len = ntohs(device_msg_length);
                                                    check = recv(comm_socket, (void*)device_msg, device_msg_len, 0);
                                                    strcpy(chat_log_entry.recipient, device_msg);

                                                    //ricezione lunghezza e messaggio e settaggio del campo relativo
                                                    memset(&device_msg, 0, sizeof(device_msg));
                                                    check = recv(comm_socket, (void*)&device_msg_length, sizeof(uint16_t), 0);
                                                    device_msg_len = ntohs(device_msg_length);
                                                    check = recv(comm_socket, (void*)device_msg, device_msg_len, 0);
                                                    strcpy(chat_log_entry.msg, device_msg);       //settaggio del messaggio
                                                                                                        
                                                    //settaggio degli altri campi di chat_log_entry
                                                    chat_log_entry.delivered = 0;
                                                    strcpy(chat_log_entry.sender, users_data.username);
                                                    time_t t = time(NULL);
                                                    struct tm *tm = localtime(&t);
                                                    sprintf(chat_log_entry.timestamp, "%d/%d/%d %d:%d:%d", tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec);
                                                    
                                                    //scrittura record su file
                                                    FILE *chat_log;
                                                    chat_log = fopen("SERVER_FILES/chat_log.bin", "ab");
                                                    fwrite(&chat_log_entry, sizeof(struct chat_log_entry), 1, chat_log);    
                                                    fclose(chat_log);
                                                    
                                                    
                                                    //AGGIORNAMENTO FILE pending_log.bin
                                                    FILE *pending_log;
                                                    FILE *pending_log_updated;
                                                    struct pending_log_entry pending_log_read;
                                                    struct pending_log_entry updated_record;
                                                    int found = FALSE;

                                                    pending_log = fopen("SERVER_FILES/pending_log.bin", "rb");
                                                    pending_log_updated = fopen("SERVER_FILES/pending_log_updated.bin", "ab");


                                                    while(fread(&pending_log_read, sizeof(struct pending_log_entry), 1, pending_log) == 1){
                                                        if(strcmp(pending_log_read.from, chat_log_entry.sender) == 0 && strcmp(pending_log_read.to, chat_log_entry.recipient) == 0){
                                                            updated_record.tot_msg = pending_log_read.tot_msg + 1;
                                                            strcpy(updated_record.from, pending_log_read.from);
                                                            strcpy(updated_record.to, pending_log_read.to);
                                                            strcpy(updated_record.most_recent_timestamp, chat_log_entry.timestamp);

                                                            fwrite(&updated_record, sizeof(struct pending_log_entry), 1, pending_log_updated);

                                                            found = TRUE;
                                                        }else{
                                                            fwrite(&pending_log_read, sizeof(struct pending_log_entry), 1, pending_log_updated);
                                                        }
                                                    }

                                                    if(!found){
                                                        updated_record.tot_msg = 1;
                                                        strcpy(updated_record.from, chat_log_entry.sender);
                                                        strcpy(updated_record.to, chat_log_entry.recipient);
                                                        strcpy(updated_record.most_recent_timestamp, chat_log_entry.timestamp);

                                                        fwrite(&updated_record, sizeof(struct pending_log_entry), 1, pending_log_updated);
                                                    }

                                                    fclose(pending_log);
                                                    fclose(pending_log_updated);

                                                    remove("SERVER_FILES/pending_log.bin");
                                                    rename("SERVER_FILES/pending_log_updated.bin", "SERVER_FILES/pending_log.bin");
                                                    
                                                }else if(recvd_command == 2){   //COMANDO \u -> CODICE 2
                                                    char recvd_msg[CREDENTIALS_LENGTH];         //contatto ricevuto dal device, da controllare
                                                    uint16_t recvd_msg_length;              //lunghezza del msg ricevuto
                                                    int recvd_msg_len;
                                                    uint16_t contact_port;      //porta del contatto ricevuto (0 se offline)

                                                    recvd_msg[0] = '\n';        //inizializzazione stringa da ricevere
                                                    

                                                    while(strcmp(recvd_msg, "END") != 0){
                                                        //controlla se recvd_contact è online. Invia la porta al device, altrimenti 0.

                                                        memset(&recvd_msg, 0, sizeof(recvd_msg));

                                                        //ricezione dimensione
                                                        check = recv(comm_socket, (void*)&recvd_msg_length, sizeof(uint16_t), 0);
                                                        manage_error(check, "Receiving contacts msg length");

                                                        //ricezione contatto
                                                        recvd_msg_len = ntohs(recvd_msg_length);
                                                        check = recv(comm_socket, (void*)recvd_msg, recvd_msg_len, 0);
                                                        manage_error(check, "Receiving contact or END msg");

                                                        if(strcmp(recvd_msg, "END") != 0){      //ricevuto un contatto
                                                            //controllo se è online
                                                            contact_port = is_online(recvd_msg);
                                                            
                                                            contact_port = htons(contact_port);
                                                            check = send(comm_socket, &contact_port, sizeof(uint16_t), 0);
                                                            manage_error(check, "Sending port to device, 0 if offline");
                                                        }
                                                    }
                                                }else if(recvd_command == 3){       //COMANDO \q -> CODICE 3
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }       //fine chat
                            
                                //OUT
                                if(strcmp(users_operation.command, "out") == 0){
                                    time_t timestamp;                    //current timestamp
                                    char *timestamp_string;             //per la conversione in stringa leggibile del timestamp corrente
                                    timestamp = time(NULL);                 //get del current timestamp e conversione a stringa leggibile dentro timestmp_string
                                    timestamp_string = ctime(&timestamp);

                                    printf(ANSI_COLOR_GREEN "%s logged out.\n" ANSI_COLOR_RESET, users_data.username);
                                    register_users_logout(users_data);
                                }
                            }
                        }else{
                            close(comm_socket);
                            exit(0);
                        }
                    }
                }
                close(comm_socket);             //servita la richiesta chiude il socket di comunicazione
                exit(0);
            }
            close(comm_socket);
        }
    }else{  //pid =/= 0 -> processo padre -> si occupa della gestione del menu e degli input dei comandi sul server
        //PROCESSO PADRE
        while(1){
            close(listening_socket);                //chiusura del socket di ascolto, non serve qui

            display_menu(server_port);              //display del menu e input utente
            do{
                menu_choice = 1;
                scanf("%s", users_choice);
                if(strcmp(users_choice, "help") != 0 && strcmp(users_choice, "list") != 0 && strcmp(users_choice, "esc") != 0){
                    menu_choice = 0;
                    printf("Digitare un comando valido tra quelli proposti.\n");
                }
            }while(menu_choice == 0);

            
            //HELP
            if(strcmp(users_choice, "help") == 0){
                help_command();
            }
            
            //LIST
            if(strcmp(users_choice, "list") == 0){
                list_command();
            }
            
            //OUT
            if(strcmp(users_choice, "esc") == 0){
                remove("SERVER_FILES/server_data.bin");     //svuota il file server_data.bin
                fclose(fopen("SERVER_FILES/server_data.bin", "wb"));

                //ACK A TUTTI I DEV CONNESSI CHE IL SERVER VA OFFLINE
                FILE *server_register;
                struct register_entry register_read;
                int i;
                char ack_msg[] = "SERVER_DISCONNECTED";
                char ack_recipient[] = "SERVER_ACK";
                struct sockaddr_in target_device_addr;
                int ack_socket;

                server_register = fopen("SERVER_FILES/server_register.bin", "rb");      //per la lettura degli utenti online

                ack_socket = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);    //socket dedicato all'operazione

                //lettura degli utenti online al momento
                while(fread(&register_read, sizeof(register_read), 1, server_register) == 1){
                    if(strcmp(register_read.timestamp_logout, "") == 0){    //utente online, il logout non è settato

                        //indirizzo destinatario
                        memset(&target_device_addr, 0, sizeof(target_device_addr));
                        target_device_addr.sin_family = AF_INET;
                        target_device_addr.sin_port = htons(register_read.port);
                        inet_pton(AF_INET, "127.0.0.1", &target_device_addr.sin_addr);

                        //invio del mittente: SERVER_ACK
                        check = sendto(ack_socket, ack_recipient, strlen(ack_recipient), 0, (struct sockaddr*)&target_device_addr, sizeof(target_device_addr));

                        //invio rapido di 3 pkt di ack al device connesso corrente
                        for(i = 0; i < DISCONNECTION_ACKS; i++){
                            check = sendto(ack_socket, ack_msg, strlen(ack_msg), 0, (struct sockaddr*)&target_device_addr, sizeof(target_device_addr));
                        }
                    }
                }

                fclose(server_register);

                return 0;
            }
        }
    }

    return 0;
}