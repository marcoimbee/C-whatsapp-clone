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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>



// -------------------- DEFINITIONS --------------------
#define CHUNK_DIM 50                //trasf file, CHUNK_DIM byte alla volta
#define CHUNK_RATE 0.1              //trasferimento di un chunk ogni CHUNK_RATE secondi
#define MAX_CONTACTS 100
#define CREDENTIALS_LENGTH 20
#define GENERIC_MSG_LENGTH 20
#define TIMESTAMP_LENGTH 20
#define BUFFER_SIZE 1024
#define ANSI_COLOR_GREEN "\x1b[32m"     //colore verde in console per i messaggi di stato
#define ANSI_COLOR_RED "\x1b[31m"   //colore rosso in console
#define ANSI_COLOR_RESET "\x1b[0m"      //reset per tornare al colore originale
#define TRUE 1
#define FALSE 0



// -------------------- GLOBAL STRUCTURES AND VARIABLES --------------------
int server_is_online;
struct file_paths{
    char folder[BUFFER_SIZE];
    char contacts[BUFFER_SIZE];
    char current_chat[BUFFER_SIZE];
    char chat_log[BUFFER_SIZE];
    char server_status[BUFFER_SIZE];
}file_paths;

struct chat_elem_entry{                     //entry del file 'chat_log.bin'
    char sender[CREDENTIALS_LENGTH];
    char recipient[CREDENTIALS_LENGTH];
    char msg[BUFFER_SIZE];
    uint8_t delivered;
    char timestamp[TIMESTAMP_LENGTH];
};

struct users_login{                         //stessa struct per signup e login, nel signup lascio srv_port = 0
    char command[CREDENTIALS_LENGTH];
    uint16_t srv_port;
    char username[CREDENTIALS_LENGTH];
    char password[CREDENTIALS_LENGTH];
    uint16_t dev_port;
};

struct current_chat_entry{                  //entry del file 'current_chat.bin'
    char username[CREDENTIALS_LENGTH];
    uint16_t port;
};

struct users_operation{                     //struct che verrà inizializzata con il comando e il parametro inserito dall'utente
    char command[BUFFER_SIZE];  //conterrà una stringa tra 'hanging', 'show', 'chat', 'share' o 'out'
    char parameter[BUFFER_SIZE];    //conterrà il secondo parametro (se presente) inserito
};

struct online_contacts_data{                //struct per i dati degli utenti online aggiungibili alla chat
    char username[CREDENTIALS_LENGTH];
    uint16_t port;
};



// -------------------- FUNCTIONS AND METHODS --------------------
/*
    void application_menu()

    Parametri:
        -> char *device_username: username con cui è stato effettuato il login sul dispositivo

    La funzione mostra quali azioni è possibile scegliere da tastiera a seconda se il server è online oppure no.
*/
void application_menu(char *device_username){
    int server_is_online = get_server_status(device_username);
    if(server_is_online){
        printf("Server ONLINE. Tutti i comandi sono disponibili.\n");
        printf("COMANDI DISPONIBILI:\n");
        printf("> hanging --> ricezione della lista di utenti che hanno inviato messaggi mentre si era offline\n");
        printf("> show username --> ricezione dei messaggi pendenti inviati da 'username'\n");
        printf("> chat username --> avvia una chat con 'username'\n");
        printf("> share file_name --> invio del file 'file_name' al/ai device con cui si sta chattando\n");
        printf("> contacts --> mostra la rubrica\n");
        printf("> add username --> aggiunge il contatto 'username' alla rubrica\n");
        printf("> out --> richiede la disconnessione dal network\n");
    }else{
        printf("Attenzione: il server è offline. Solo alcuni comandi saranno disponibili.\n");
        printf("COMANDI DISPONIBILI:\n");
        printf("> contacts --> mostra la rubrica\n");
        printf("> out --> richiede la disconnessione dal network\n");
    }
}


/*
    uint16_t get_server_port()

    Parametri:
        -> char *command: comando scelto dall'utente, o 'signup' o 'in'
        -> uint16_t chosen_port: porta specificata nel caso di input del comando 'in'

    La funzione restituisce il numero di porta su cui il server è in ascolto.
    Si legge dal file 'SERVER_FILES/server_data.bin'.
*/
uint16_t get_server_port(char *command, uint16_t chosen_port){
    FILE *server_data;
    uint16_t server_port_read = 0;

    server_data = fopen("SERVER_FILES/server_data.bin", "rb");

    if(strcmp(command, "signup") == 0){
        while(fread(&server_port_read, sizeof(uint16_t), 1, server_data) == 1){
            fclose(server_data);
            return server_port_read;        //return della porta (nel file) su cui sta ascoltando il server
        }
        fclose(server_data);
        return 0;       //se non si fa il return dal while, il server non è in funzione e si ritorna 0
    }

    if(strcmp(command, "in") == 0){
        while(fread(&server_port_read, sizeof(uint16_t), 1, server_data) == 1){
            if(server_port_read == chosen_port){        //se la trova, ritorna il valore
                fclose(server_data);
                return server_port_read;            //return il valore della porta trovata nel file, uguale a quella specificata dall'utente, se c'è vi è in ascolto il server
            }
        }
        fclose(server_data);
        return 0;       //se non si fa il return dal while, il server non è in ascolto sulla porta chosen_port e si ritorna 0
    }

    return -1;          //non si arriva mai qui
}


/*
    void display_active_server()

    Display a video della porta su cui è possibile connettersi al server in esecuzione (se c'è)
*/
void display_active_server(){
    FILE *server_data;
    uint16_t server_port;
    int counter = 0;

    printf(ANSI_COLOR_GREEN);
    printf( "> Server in ascolto sulla porta: ");

    server_data = fopen("SERVER_FILES/server_data.bin", "rb");

    while(fread(&server_port, sizeof(uint16_t), 1, server_data) == 1){
        printf("%d; ", server_port);
        counter++;
    }

    if(counter == 0){
        printf(ANSI_COLOR_RED "Nessun server attualmente attivo.\n" ANSI_COLOR_RESET);
    }else{
        printf("\n");
    }

    fclose(server_data);
    printf(ANSI_COLOR_RESET);
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
    int check_if_chatting()

    Parametri:
        -> char *dev_username: username con cui è stato effettuato il login sul dispositivo
        -> char *from_username: username dell'utente oggetto di controllo

    La funzione restituisce 1 se l'username passato è presente nel file
    'DEVICES/dev_username/active_chat.bin', ovvero se è uno degli utenti con cui chi
    ha eseguito l'accesso al device sta chattando al momento.
    Restituisce 0 altrimenti.
*/
int check_if_chatting(char *dev_username, char *from_username){
    FILE *current_chat;
    struct current_chat_entry current_chat_read;
    char current_chat_path[BUFFER_SIZE];

    sprintf(current_chat_path, "%s%s%s", "DEVICES/", dev_username, "/current_chat.bin");        //composizione percorso

    current_chat = fopen(current_chat_path, "rb");

    while(fread(&current_chat_read, sizeof(struct current_chat_entry), 1, current_chat) == 1){
        if(strcmp(current_chat_read.username, from_username) == 0){
            fclose(current_chat);
            return 1;               //trovato
        }
    }

    fclose(current_chat);
    return 0;                   //non trovato
}


/*
    int is_friend()

    Parametri:
        -> char *dev_username: username con cui è stato effettuato il login sul dispositivo
        -> char *to_be_checked: username dell'utente oggetto di controllo

    Controlla se l'username to_be_checked è registrato nella rubrica di dev_username (= è amico).
    Ritorna 1 se lo è, 0 altrimenti.
*/
uint8_t is_friend(char *dev_username, char *to_be_checked){
    FILE *contacts;                     //file rubrica
    char contacts_path[BUFFER_SIZE];
    char contacts_read[CREDENTIALS_LENGTH];     //lettura dal file rubrica

    sprintf(contacts_path, "%s%s%s", "DEVICES/", dev_username, "/contacts.bin");     //composizione percorso file contacts.bin

    contacts = fopen(contacts_path, "rb");

    while(fread(&contacts_read, sizeof(contacts_read), 1, contacts) == 1){      //lettura file contatti
        if(strcmp(to_be_checked, contacts_read) == 0){
            fclose(contacts);
            return 1;               //l'utente con cui si vuole chattare è registrato in rubrica
        }
    }

    fclose(contacts);

    return 0;           //to_be_checked non è registrato nella rubrica di logged_user
}


/*
    void broadcast_msg()

    Parametri:
        -> char *msg_type: tipo di messaggio che si invia al/ai partecipante/i
        -> char *chat_input: messaggio da inviare al/ai membro/i della chat
        -> char *device_username: mittente del messaggio
        -> int chat_socket: socket UDP su cui inviare il messaggio
        -> struct online_coontacts_data added_user: struct contenente i dati di chi si aggiunge alla chat, ha significato solo se msg_type è "ADD"
        -> uint8_t save_this_msg: indica a chi riceve il msg se deve salvarlo oppure no

    La funzione scorre i record del file current_chat.bin dell'utente e 
    per ogni record invia il messaggio sulla porta associata.
*/
void broadcast_msg(char *msg_type, char *chat_input, char *device_username, int chat_socket, struct online_contacts_data added_user, uint8_t save_this_msg){
    FILE *current_chat;
    struct current_chat_entry current_chat_read;
    struct sockaddr_in target_addr;         //struttura per il target device
    int check;
    char current_chat_path[BUFFER_SIZE];
    char final_msg[BUFFER_SIZE];

    sprintf(current_chat_path, "%s%s%s", "DEVICES/", device_username, "/current_chat.bin");

    current_chat = fopen(current_chat_path, "rb");

    //invio ad ogni componente della chat di gruppo
    while(fread(&current_chat_read, sizeof(struct current_chat_entry), 1, current_chat) == 1){
        //creazione indirizzo target device
        memset(&target_addr, 0, sizeof(target_addr));
        target_addr.sin_family = AF_INET;
        target_addr.sin_port = htons(current_chat_read.port);
        inet_pton(AF_INET, "127.0.0.1", &target_addr.sin_addr);

        //invio username mittente
        check = sendto(chat_socket, device_username, strlen(device_username), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
        if(check < 0){
            printf("Errore durante l'invio a %s:%d!\n", current_chat_read.username, current_chat_read.port);
        }
        
        //a seconda del tipo di messaggio si compone un pkt diverso
        if(strcmp(msg_type, "CHAT") == 0){      //messaggio normale da inviare a tutti, si invia solo msg_type per ora
            sprintf(final_msg, "%s %d", msg_type, save_this_msg);   
        }else if(strcmp(msg_type, "ADD") == 0){     //aggiunta di un partecipante
            sprintf(final_msg, "%s %s %d", msg_type, added_user.username, added_user.port);       //la struct add_to_chat ha significato
        }else if(strcmp(msg_type, "QUIT") == 0){
            sprintf(final_msg, "%s %s", msg_type, device_username);      //basta inviare il tipo di messaggio e l'username dell'utente che vuole uscire dalla chat
        }else if(strcmp(msg_type, "FILENAME") == 0){
            sprintf(final_msg, "%s %s", msg_type, chat_input);      //invio del tipo di msg e del filename
        }

        /*
            Quando msg_type vale:
                -> 'CHAT': si procede a inviare il messaggio vero e proprio, prima si manda questo pacchetto con msg_type|save_this_msg
                    -> save_this_msg: indica al destinatario se deve salvare o meno il messaggio nel suo log
                
                -> 'ADD': si aggiunge un partecipante alla chat corrente
                    -> add_user.username: username dell'utente aggiunto
                    -> add_user.port: porta su cui è raggiungibile l'utente aggiunto
                
                -> 'QUIT': si esce dalla chat corrente
                    -> device_username: username dell'utente uscito dalla chat
                
                -> 'FILENAME': si sta per iniziare l'invio di un file ai partecipanti della chat
                    -> chat_input: contiene il nome del file che si sta per inviare
        */

        //invio pkt
        check = sendto(chat_socket, final_msg, strlen(final_msg), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
        if(check < 0){
            printf("Errore durante l'invio a %s:%d!\n", current_chat_read.username, current_chat_read.port);
        }
        
        //invio del msg vero e proprio, se msg_type = CHAT
        if(strcmp(msg_type, "CHAT") == 0){
            check = sendto(chat_socket, chat_input, strlen(chat_input), 0, (struct sockaddr*)&target_addr, sizeof(target_addr));
            if(check < 0){
                printf("Errore durante l'invio a %s:%d!\n", current_chat_read.username, current_chat_read.port);
            }
        }
    }

    fclose(current_chat);
}


/*
    void add_to_chat()

    Parametri:
        -> char *device_username: chi aggiunge added_username
        -> char *added_username: utente da aggiungere alla chat in corso
        -> uint16_t added_port: porta sulla quale è raggiungibile added_username
    
    La funzione registra nel file current_chat.bin il nuovo utente a cui inviare
    i messaggi della chat.
*/
void add_to_chat(char *device_username, char *added_username, uint16_t added_port){
    FILE *current_chat;
    char current_chat_path[BUFFER_SIZE];
    struct current_chat_entry added_entry;

    strcpy(added_entry.username, added_username);
    added_entry.port = added_port;

    sprintf(current_chat_path, "%s%s%s", "DEVICES/", device_username, "/current_chat.bin");

    current_chat = fopen(current_chat_path, "ab");

    fwrite(&added_entry, sizeof(struct current_chat_entry), 1, current_chat);

    fclose(current_chat);
}


/*
    void update_server_status()

    Parametri:
        -> int status: stato da scrivere sul file: 0 se server offline, 1 se online
        -> char *device_username: chi aggiorna il suo file server_status.bin

    La funzione registra sul file server_styatus.bin lo stato passato come parametro.
*/
void update_server_status(int status, char *device_username){
    FILE *server_status;
    char server_status_path[BUFFER_SIZE];

    struct server_status_data{
        int status;
    }server_status_data;

    server_status_data.status = status;

    sprintf(server_status_path, "%s%s%s", "DEVICES/", device_username, "/server_status.bin");
    server_status = fopen(server_status_path, "wb");

    //si scrive status nel file: 0 se il server è andato offline, 1 se online
    fwrite(&server_status_data, sizeof(struct server_status_data), 1, server_status);  

    printf("Scritto su file: %d\n", server_status_data.status);

    fclose(server_status);
}


/*
    int get_server_status()

    Parametri:
        -> char *device_username: chi legge il suo file server_status.bin

    La funzione legge il file server_status.bin e ne restituisce il contenuto.
    1 se il server è online, 0 se offline.
*/
int get_server_status(char *device_username){
    FILE *server_status;
    char server_status_path[BUFFER_SIZE];
    
    struct server_status_data{
        int status;
    }server_status_data;


    sprintf(server_status_path, "%s%s%s", "DEVICES/", device_username, "/server_status.bin");
    server_status = fopen(server_status_path, "rb");

    //si legge status nel file: 0 se il server è andato offline, 1 se online
    fread(&server_status_data, sizeof(struct server_status_data), 1, server_status);  

    fclose(server_status);

    return server_status_data.status;
}


/*
    void quit_current_chat()

    Parametri:
        -> int chat_socket: identificatore del socket usato per la chat
        -> struct users_login login_data: dati di accesso del device corrente

    La funzione fa uscire chi la invoca dalla chat corrente.
*/
void quit_current_chat(int chat_socket, struct users_login login_data){
    /*
        USCITA DALLA CHAT CORRENTE

        Si invia un ACK di uscita dalla chat dell'utente agli altri utenti della chat
        Si svuota il file current_chat.bin.
        Si torna al menu iniziale.
    */
    FILE *new_current_chat;
    char current_chat_path[BUFFER_SIZE];

    system("clear");

    //Si manda broadcast un ack di uscita dell'utente dalla chat
    struct online_contacts_data null_user;    //per completare gli argomenti, non serve
    broadcast_msg("QUIT", "", login_data.username, chat_socket, null_user, 0);     
    
    sprintf(current_chat_path, "%s%s%s", "DEVICES/", login_data.username, "/current_chat.bin");

    remove(current_chat_path);        //cancellazione del contenuto del file

    new_current_chat = fopen(current_chat_path, "wb");    //ricreazione del file
    fclose(new_current_chat);
}


/*
    void save_to_log()

    Parametri:
        -> struct chat_elem_entry to_be_saved: struct con i dati da registrare in 'chat_log.bin'
        -> char *device_username: chi scrive sul suo file 'chat_log.bin'

    La funzione salva su file il messaggio scambiato con il target.
*/
void save_to_log(struct chat_elem_entry to_be_saved, char *device_username){
    FILE *chat_log;
    char chat_log_path[BUFFER_SIZE];

    to_be_saved.msg[strlen(to_be_saved.msg) - 1] = '\0';

    sprintf(chat_log_path, "%s%s%s", "DEVICES/", device_username, "/chat_log.bin");

    chat_log = fopen(chat_log_path, "ab");

    fwrite(&to_be_saved, sizeof(struct chat_elem_entry), 1, chat_log);

    fclose(chat_log);
}


/*
    void display_old_messages()

    Parametri:
        char *target_user: utente target della chat
        char *device_username: username iniziatore della chat con target_user
    
    La funzione visualizza i messaggi passati scambiati tra device_username e target_user
*/
void display_old_messages(char *target_user, char *device_username){
    FILE *chat_log;
    char chat_log_path[BUFFER_SIZE];
    struct chat_elem_entry chat_log_read;

    sprintf(chat_log_path, "%s%s%s", "DEVICES/", device_username, "/chat_log.bin");     //composizione percorso file

    chat_log = fopen(chat_log_path, "rb");

    while(fread(&chat_log_read, sizeof(struct chat_elem_entry), 1, chat_log) == 1){     //lettura record
        //se si trovano record in cui il mittente è l'utente loggato con destinatario il target scelto o record in cui il mittente è il target con destinatario l'utente loggato
        if((strcmp(chat_log_read.sender, device_username) == 0 && strcmp(chat_log_read.recipient, target_user) == 0) || (strcmp(chat_log_read.sender, target_user) == 0 && strcmp(chat_log_read.recipient, device_username) == 0)){
            //se il mittente è l'utente loggato
            if(strcmp(chat_log_read.sender, device_username) == 0){
                if(chat_log_read.delivered){        //se è consegnato
                    printf("> Tu**: %s\n", chat_log_read.msg);
                }else{                              //se non è consegnato
                    printf("> Tu*: %s\n", chat_log_read.msg);
                }
            //se il mittente è l'utente target
            }else{
                if(chat_log_read.delivered){        //stesso discorso
                    printf("> %s**: %s\n", chat_log_read.sender, chat_log_read.msg);
                }else{
                    printf("> %s*: %s\n", chat_log_read.sender, chat_log_read.msg);
                }
            }
        }
    }

    fclose(chat_log);
}



// --------------------  MAIN  -------------------
int main(int argc, char* argv[]){

    //DEVICE PARAMETERS VARs
    int port;
    uint32_t device_port;
    
    //CLIENT/SERVER COMMUNICATION VARs
    struct sockaddr_in server_address;
    int conn_socket;
    struct users_login login_data;          //struct che verrà inizializzata con i dati da inviare al server in fase di signup o in
    char login_msg[BUFFER_SIZE];            //stringa serializzata con i dati di login: comando, param1, param2 (, param3)
    uint16_t msg_len;                       //lunghezza di login_msg, operation_msg che verrà inviata al server
    uint8_t outcome_op;                     //esito dell'operazione
    int logged_in;                          //utente associato al device ha eseguito il login (1) oppure no (0)
    int len;                                //lunghezza della stringa login_msg e operation_msg
    int srv_port_ok;                        //porta srv_port scelta dall'utente valida o no
    char users_input[BUFFER_SIZE];
    uint8_t server_disconnected;
    
    //CLIENT UI VARs
    int menu_choice;

    //UTILITY VARs
    int check;
    char *split_input;          
    int index_command;


    //RECUPERO PORTA DEVICE
    if(argv[1] != NULL){            //porta del processo device
        device_port = atoi(argv[1]);        //recupero del parametro specificato all'avvio del client
    }else{
        printf(ANSI_COLOR_RED "Attenzione: specificare una porta da associare al device (./dev <port>).\n" ANSI_COLOR_RESET);   //messaggio di errore se non si specifica una porta
        return 0;
    }


    /*
        All'avvio, si mostra un menu con i comandi signup e in.

        'signup user pwd':
            Si recupera dal file 'server_data.bin' la porta su cui è in ascolto il server.
            Lo si fa ad ogni ciclo perchè nel frattempo il server potrebbe essersi disconnesso.
            Si stabilisce la connessione con il server in ascolto su quella porta che servirà la richiesta di signup.
            Si invia al server una stringa serializzata contenente 'signup porta user pwd'.
            Il server controlla che l'username non sia già occupato. Se sì, ritorna il valore 0, che viene assegnato a outcome.
            Se l'username è valido, il server registra in un suo file l'associazione username-password dell'utente.
            Un utente può scegliere di eseguire 'signup' quante volte vuole, purchè scelga username non occupati ogni volta.
            Per ogni operazione di signup si instaura una connessione TCP con il srever, che viene chiusa a richiesta servita.

        'in srv_port user pwd':
            Si controlla da file se la porta srv_port è valida. Se no si visualizza un messaggio di errore e si ritorna al menu.
            In caso di srv_port valida, si stabilisce una connessione con il server.
            Si manda al server una stringa serializzata contenente 'in porta user pwd'.
            Se l'username esiste, e la password pwd ad esso associata corrisponde, può essere eseguito il login.
            Altrimenti, ritorna il valore 0, che viene assegnato ad outcome.
            Il server, in caso di credenziali valide, registra il record 'user_porta_loginTimestamp_logoutTimestamp' 
            (loginTimestamp sarà pari al current_timestamp e logoutTimestamp sarà NULL, essendo username adesso online) in un suo file.
            Ad ogni operazione di 'in' con credenziali valide, viene cancellato il vecchio record relativo all'username dal file interno del server,
            e se ne aggiunge uno aggiornato con la nuova porta scelta, il timestampLogin aggiornato e il timestampLogout NULL.
            Avvenuta con successo l'operazione di login, si instaura una connessione TCP con il server che viene mantenuta finchè il device non esegue
            'out' o si disconnette senza preavviso.
    */
    do{
        uint16_t server_port = 0;               //porta del server con cui comunicare
        
        display_active_server();    //DISPLAY DEL SERVER IN ASCOLTO

        printf("\n");
        printf("COMANDI DISPONIBILI:\n");
        printf("> signup username password --> crea un nuovo account sul server\n");
        printf("> in srv_port username password --> accede all'account specificato comunicando con il server in ascolto su 'srv_port'\n");
        do{
            /*
                users_input: stringa inserita dall'utente (comando param param (param))
                j: usata nel ciclo for
                split_key: indica di dividere la stringa fino allo spazio
                index_command: indica dove si trova il primo spazio
                menu_choice: usata per ciclare sul menu se si sceglie un'opzione invalida
            */
            int j;
            menu_choice = 1;
            char split_key[] = " ";         //divisore dell'input utente

            printf("Digitare un comando: ");

            fgets(users_input, BUFFER_SIZE, stdin);             //input utente in users_input

            index_command = strcspn(users_input, split_key);

            memset(&login_data, 0, sizeof(login_data));
            for(j = 0; j < index_command; j++){                 //copio fino a index_command - 1
                login_data.command[j] = users_input[j];         //il comando inserito si trova in login_data.command
            }

            if(strcmp(login_data.command, "signup") != 0 && strcmp(login_data.command, "in") != 0){
                menu_choice = 0;
                printf(ANSI_COLOR_RED "Digitare un comando valido.\n" ANSI_COLOR_RESET);
            }
        }while(menu_choice == 0);

        //scelta di 'signup'
        if(strcmp(login_data.command, "signup") == 0){
            server_port = get_server_port("signup", 0);            //recupero della porta del primo server disponibile in ascolto, il secondo parametro non importa qui
            if(server_port == 0){
                printf(ANSI_COLOR_RED "Il server è attualmente offline. Riprova più tardi.\n" ANSI_COLOR_RESET);
                exit(-1);
            }else{                
                char *splitter;
                int counter = 0;
                splitter = strtok(users_input, " ");        //divide la stringa nelle parole inserite
                while(splitter != NULL){                    //finchè non si arriva a fine stringa
                    switch(counter){
                        case 0:
                            counter++;                      //il comando è già stato recuperato prima e si trova in login_data.command
                            break;
                        case 1:
                            strcpy(login_data.username, splitter);     //copia dell'username
                            counter++;
                            break;
                        case 2:
                            strcpy(login_data.password, splitter);     //copia della password
                            break;
                    }
                    splitter = strtok(NULL, " ");
                }

                //APERTURA SOCKET DI CONNESSIONE PER signup
                conn_socket = socket(AF_INET, SOCK_STREAM, 0);
                if(conn_socket < 0){
                    perror("Errore durante la creazione del socket: \n");
                    exit(-1);
                }
                
                //connessione al server in ascolto sulla porta server_port
                memset(&server_address, 0, sizeof(server_address));
                server_address.sin_family = AF_INET;
                
                server_address.sin_port = htons(server_port);
                inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

                check = connect(conn_socket, (struct sockaddr*)&server_address, sizeof(server_address));
                if(check < 0){
                    perror("Errore nella connessione con il server: \n");
                    return 0;
                }

                //preparazione dei dati da inviare al server
                memset(&login_msg, 0, sizeof(login_msg));
                sprintf(login_msg, "%s %s %s", login_data.command, login_data.username, login_data.password);       //serializza i dati in login_msg, la porta non si invia
                
                //invio dimensione stringa - invio stringa - ricezione outcome
                len = strlen(login_msg);      //calcolo lunghezza messaggio  
                msg_len = htons(len);         //conversione della lunghezza del messaggio
                check = send(conn_socket, (void*)&msg_len, sizeof(uint16_t), 0);    //invio dimensione
                manage_error(check, "Sending SIGNUP msg length");
                check = send(conn_socket, (void*)login_msg, strlen(login_msg), 0);  //invio stringa dei dati inseriti
                manage_error(check, "Sending SIGNUP serialized msg");

                //gestione dell'esito dell'operazione
                check = recv(conn_socket, (void*)&outcome_op, sizeof(uint8_t), 0);       //ricezione dell'esito dal server sotto forma di uint8_t
                manage_error(check, "Receiving SIGNUP outcome");

                close(conn_socket);     //chiusura del socket una volta ricevuto l'esito dell'operazione di signup
            }
        }
        
        //scelta di 'in'
        if(strcmp(login_data.command, "in") == 0){
            char *splitter;
            int counter = 0;
            splitter = strtok(users_input, " ");        //divide la stringa nelle parole inserite
            while(splitter != NULL){                    //finchè non si arriva a fine stringa
                switch(counter){
                    case 0:
                        counter++;                      //il comando è già stato recuperato prima
                        break;
                    case 1:
                        login_data.srv_port = atoi(splitter);

                        //verifica della porta scelta
                        server_port = get_server_port("in", login_data.srv_port);            //recupero della porta del primo server disponibile in ascolto, il secondo parametro non importa qui
                        if(server_port == 0){       //server_port vale 0 se la porta inserita è invalida, assume come valore la porta del server scelto se questa è valida
                            srv_port_ok = 0;     //se la porta non va bene si usa outcome_op per evitare di connettersi al server e tornare invece al menu iniziale
                        }else{
                            srv_port_ok = 1;     //se la porta è valida questo è il discriminante per eseguire le op con il server
                        }     

                        counter++;
                        break;
                    case 2:
                        strcpy(login_data.username, splitter);     //copia dell'username
                        counter++;
                        break;
                    case 3:
                        strcpy(login_data.password, splitter);     //copia della password
                        break;
                }
                splitter = strtok(NULL, " ");
            }
            login_data.dev_port = device_port;          //copia adesso anche della porta associata al processo device in corso

            if(srv_port_ok == 1){           //se la porta scelta è valida si contatta il server per il login
                //APERTURA SOCKET DI CONNESSIONE PER in
                conn_socket = socket(AF_INET, SOCK_STREAM, 0);
                if(conn_socket < 0){
                    perror("Errore durante la creazione del socket: \n");
                    exit(-1);
                }

                //connessione al server in ascolto sulla porta server_port
                memset(&server_address, 0, sizeof(server_address));
                server_address.sin_family = AF_INET;
                
                server_address.sin_port = htons(server_port);
                inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

                check = connect(conn_socket, (struct sockaddr*)&server_address, sizeof(server_address));
                if(check < 0){
                    perror("Errore nella connessione con il server: \n");
                    return 0;
                }

                //preparazione dei dati da inviare al server
                memset(&login_msg, 0, sizeof(login_msg));
                sprintf(login_msg, "%s %d %s %s", login_data.command, login_data.dev_port, login_data.username, login_data.password);       //serializza i dati in login_msg, la porta non si invia

                //invio dimensione stringa - invio stringa - ricezione outcome
                len = strlen(login_msg);      //calcolo lunghezza messaggio  
                msg_len = htons(len);         //conversione della lunghezza del messaggio
                check = send(conn_socket, (void*)&msg_len, sizeof(uint16_t), 0);    //invio dimensione
                manage_error(check, "Sending IN msg length");
                check = send(conn_socket, (void*)login_msg, strlen(login_msg), 0);  //invio stringa dei dati inseriti
                manage_error(check, "Sending serialized IN msg");

                //gestione dell'esito dell'operazione
                check = recv(conn_socket, (void*)&outcome_op, sizeof(uint8_t), 0);       //ricezione dell'esito dal server sotto forma di uint8_t
                manage_error(check, "receiving IN outcome");
                //non si chiude il socket perchè si continua ad usare per le comunicazioni future col server

                server_disconnected = 0;
            }
        }

        //gestione dell'esito nei due casi
        if(strcmp(login_data.command, "signup") == 0){
            if(outcome_op == 0){
                printf(ANSI_COLOR_RED "Username '%s' occupato, sceglierne un altro.\n" ANSI_COLOR_RESET, login_data.username);
            }else{
                /*
                    COMPOSIZIONE CARTELLA PERSONALE UTENTE:
                        DEVICES/username/
                            -> contacts.bin -> lista username amici
                            -> current_chat.bin -> username degli amici nella chat corrente
                            -> chat_log.bin -> storico dei messaggi scambiati
                            -> server_status.bin -> stato attuale del server, contiene 1 se online, 0 se offline
                */                
                FILE *create_file;

                sprintf(file_paths.folder, "%s%s", "DEVICES/", login_data.username);

                mkdir(file_paths.folder, 0777);       //creazione cartella personale
                
                sprintf(file_paths.contacts, "%s%s%s", "DEVICES/", login_data.username, "/contacts.bin");
                create_file = fopen(file_paths.contacts, "wb");        //creazione file rubrica personale nella cartella appena creata
                fclose(create_file);

                sprintf(file_paths.current_chat, "%s%s%s", "DEVICES/", login_data.username, "/current_chat.bin");
                create_file = fopen(file_paths.current_chat, "wb");
                fclose(create_file);

                sprintf(file_paths.chat_log, "%s%s%s", "DEVICES/", login_data.username, "/chat_log.bin");
                create_file = fopen(file_paths.chat_log, "wb");
                fclose(create_file);

                sprintf(file_paths.server_status, "%s%s%s", "DEVICES/", login_data.username, "/server_status.bin");
                create_file = fopen(file_paths.server_status, "wb");
                fclose(create_file);

                printf(ANSI_COLOR_GREEN "Utente registrato. E' adesso possibile eseguire il login come '%s'.\n" ANSI_COLOR_RESET, login_data.username);
                printf(ANSI_COLOR_GREEN "Creata cartella personale.\n" ANSI_COLOR_RESET);
                }
            logged_in = 0;                  //si torna al menu iniziale
        }

        if(strcmp(login_data.command, "in") == 0){
            if(!srv_port_ok){
                printf(ANSI_COLOR_RED "> Nessun server in ascolto sulla porta '%d'. Scegliere una porta valida.\n" ANSI_COLOR_RESET, login_data.srv_port);
                logged_in = 0;
            }else if(outcome_op == 0){
                printf(ANSI_COLOR_RED "Username o password errati.\n" ANSI_COLOR_RESET);
                logged_in = 0;
            }else{
                printf(ANSI_COLOR_GREEN "Accesso eseguito come '%s'.\n" ANSI_COLOR_RESET, login_data.username);
                logged_in = 1;          //si continua nel programma senza tornare al menu, il login è andato a buon fine

                update_server_status(1, login_data.username);        //scrive nel file dedicato che il server è online
            }
        }
    }while(outcome_op == 0 || logged_in == 0);


    /*
        LOGIN AVVENUTO CON SUCCESSO
    */

    system("clear");

    pid_t dev_pid;

    dev_pid = fork();

    //PROCESSO FIGLIO
    if(dev_pid == 0){
        uint16_t serialized_operation_length;
        int serialized_operation_len;
        struct users_operation users_operation;
        char serialized_operation[BUFFER_SIZE];

        do{
            char *splitter;
            int counter = 0;        //variabili per la divisione della stringa inserita

            memset(&users_input, 0, sizeof(users_input));       //pulizia del buffer che conterrà l'input utente
            memset(&users_operation, 0, sizeof(users_operation));
            fflush(stdin);

            //menu mostrato all'utente una volta eseguito il login
            printf(ANSI_COLOR_GREEN "Accesso eseguito come %s.\n" ANSI_COLOR_RESET, login_data.username);
            application_menu(login_data.username);          //a seconda dello stato del server, i comandi cambiano
            
            //CICLO DI INSERIMENTO UTENTE
            do{
                /*
                    users_input: stringa inserita dall'utente (comando param (param))
                    j: usata nel ciclo for
                    split_key: indica di dividere la stringa fino allo spazio
                    index_command: indica dove si trova il primo spazio
                    menu_choice: usata per ciclare sul menu se si sceglie un'opzione invalida
                */
                int j;
                char split_key[] = " ";         //divisore dell'input utente

                printf("Digitare un comando: ");

                memset(&users_input, 0, sizeof(users_input));
                fgets(users_input, BUFFER_SIZE, stdin);         //input utente in users_input

                index_command = strcspn(users_input, split_key);

                for(j = 0; j < index_command; j++){         //copio fino a index_command - 1
                    users_operation.command[j] = users_input[j];         //il comando inserito si trova in users_operation.command
                }

                server_is_online = get_server_status(login_data.username);      //a secodna dello stato del server si mostrano i comandi disponibili
                if(server_is_online){
                    //comandi seguiti da un parametro
                    if(strcmp(users_operation.command, "chat") == 0 || strcmp(users_operation.command, "show") == 0 || strcmp(users_operation.command, "share") == 0 || strcmp(users_operation.command, "add") == 0){      
                        splitter = strtok(users_input, " ");        //divide la stringa nelle parole inserite
                        while(splitter != NULL){               //finchè non si arriva a fine stringa
                            switch(counter){
                                case 0:
                                    counter++;              //il comando è già stato recuperato prima
                                    break;
                                case 1:
                                    strcpy(users_operation.parameter, splitter);        //recupero del parametro
                                    break;
                            }
                            splitter = strtok(NULL, " ");
                        } 
                        menu_choice = 1;
                        users_operation.parameter[strcspn(users_operation.parameter, "\n")] = 0;
                    }else if(strcmp(users_operation.command, "hanging\n") == 0 || strcmp(users_operation.command, "contacts\n") == 0 || strcmp(users_operation.command, "out\n") == 0){       //comandi senza parametro
                        menu_choice = 1;
                        users_operation.command[strcspn(users_operation.command, "\n")] = 0;
                    }else{
                        menu_choice = 0;
                        printf(ANSI_COLOR_RED "Digitare un comando valido.\n" ANSI_COLOR_RESET);
                    }
                }else{
                    //server offline: solo alcuni comandi sono disponibili (quelli per cui non serve contattare il server)
                    if(strcmp(users_operation.command, "contacts\n") == 0 || strcmp(users_operation.command, "out\n") == 0){
                        menu_choice = 1;
                        users_operation.command[strcspn(users_operation.command, "\n")] = 0;
                    }else{
                        menu_choice = 0;
                        printf(ANSI_COLOR_RED "Digitare un comando valido.\n" ANSI_COLOR_RESET);
                    }
                }
            }while(menu_choice == 0);   


            //INVIO INPUT AL SERVER (solo per le azioni per cui è necessaria l'azione del server)
            if(strcmp(users_operation.command, "hanging") == 0 || strcmp(users_operation.command, "show") == 0 || strcmp(users_operation.command, "chat") == 0 || strcmp(users_operation.command, "share") == 0 || strcmp(users_operation.command, "add") == 0 || (strcmp(users_operation.command, "out") == 0 && get_server_status(login_data.username))){
                memset(&serialized_operation, 0, sizeof(serialized_operation));

                //serializzazione dati a seconda del comando inserito
                if(strcmp(users_operation.command, "show") == 0 || strcmp(users_operation.command, "chat") == 0 || strcmp(users_operation.command, "share") == 0 || strcmp(users_operation.command, "add") == 0){  
                    sprintf(serialized_operation, "%s %s", users_operation.command, users_operation.parameter);
                }else{
                    sprintf(serialized_operation, "%s", users_operation.command);
                }

                //calcolo dimensione - invio dimensione - invio stringa serializzata
                serialized_operation_len = strlen(serialized_operation);
                serialized_operation_length = htons(serialized_operation_len);
                check = send(conn_socket, (void*)&serialized_operation_length, sizeof(uint16_t), 0);
                manage_error(check, "Sending operation string length");
                check = send(conn_socket, (void*)serialized_operation, serialized_operation_len, 0);
                manage_error(check, "Sending serialized operation string");
            }

            
            //SHOW username -> serve contattare il server
            if(strcmp(users_operation.command, "show") == 0){
                printf("\n");

                uint16_t target_ok;                 //discriminante per target valido o meno

                check = recv(conn_socket, &target_ok, sizeof(uint16_t), 0);     //ricezione della risposta relativa al controllo del target specificato
                manage_error(check, "Receiving outcome on choice of target username");
                target_ok = ntohs(target_ok);

                if(target_ok){          //username esistente nel sistema
                    printf(ANSI_COLOR_GREEN "Messaggi ricevuti da %s mentre eri offline:\n" ANSI_COLOR_RESET, users_operation.parameter);

                    FILE *chat_log;
                    char chat_log_path[BUFFER_SIZE];
                    char recvd_data[BUFFER_SIZE];
                    uint16_t recvd_data_length;
                    int recvd_data_len;
                    struct chat_elem_entry chat_log_entry;
                    int counter = 0;

                    sprintf(chat_log_path, "%s%s%s", "DEVICES/", login_data.username, "/chat_log.bin");

                    chat_log = fopen(chat_log_path, "ab");

                    //si ricevono i record dal server e si mostrano. Quando il server invia 'END_SHARE' si esce dal ciclo
                    while(TRUE){
                        //ricezione lunghezza e msg OPPURE ack di fine trasmissione
                        memset(&recvd_data, 0, sizeof(recvd_data));
                        check = recv(conn_socket, (void*)&recvd_data_length, sizeof(uint16_t), 0);
                        recvd_data_len = ntohs(recvd_data_length);
                        check = recv(conn_socket, (void*)recvd_data, recvd_data_len, 0);

                        //controllo ack di fine trasmissione
                        if(strcmp(recvd_data, "END_SHARE") == 0){
                            break;
                        }

                        counter++;
                        printf("    -> %s, ", recvd_data);      //display del messaggio

                        strcpy(chat_log_entry.msg, recvd_data);

                        //ricezione lunghezza e timestamp
                        memset(&recvd_data, 0, sizeof(recvd_data));
                        check = recv(conn_socket, (void*)&recvd_data_length, sizeof(uint16_t), 0);
                        recvd_data_len = ntohs(recvd_data_length);
                        check = recv(conn_socket, (void*)recvd_data, recvd_data_len, 0);

                        printf("%s\n", recvd_data);     //display del timestamp di ricezione

                        strcpy(chat_log_entry.sender, users_operation.parameter);
                        strcpy(chat_log_entry.recipient, login_data.username);
                        strcpy(chat_log_entry.timestamp, recvd_data);
                        chat_log_entry.delivered = 1;

                        fwrite(&chat_log_entry, sizeof(struct chat_elem_entry), 1, chat_log);       //salvataggio nel log
                    }

                    if(counter == 0){
                        printf("    -> Non ci sono messaggi da visualizzare.\n");
                    }

                    fclose(chat_log);
                }else{
                    printf(ANSI_COLOR_RED "> Nessun utente con username '%s' registrato nel sistema.\n" ANSI_COLOR_RESET, users_operation.parameter);
                }

                printf("\n");
            }
            
            //HANGING -> serve contattare il server
            if(strcmp(users_operation.command, "hanging") == 0){
                printf("\n");
                printf(ANSI_COLOR_GREEN "> MESSAGGI PENDENTI:\n" ANSI_COLOR_RESET);

                char sender[CREDENTIALS_LENGTH];
                int total;
                char timestamp[TIMESTAMP_LENGTH];

                char serialized_data[BUFFER_SIZE];
                uint16_t serialized_data_length;
                int serialized_data_len;

                int counter = 0;

                //si ricevono i record dal server e si mostrano. Quando il server invia 'END_PENDING' si esce dal ciclo
                while(TRUE){
                    memset(&serialized_data, 0, sizeof(serialized_data));

                    //ricezione mittente e tot messaggi dal server OPPURE ack di fine trasmissione
                    check = recv(conn_socket, (void*)&serialized_data_length, sizeof(uint16_t), 0);
                    serialized_data_len = ntohs(serialized_data_length);
                    check = recv(conn_socket, (void*)serialized_data, serialized_data_len, 0);

                    //controllo end ack
                    if(strcmp(serialized_data, "END_PENDING") == 0){
                        break;
                    }

                    //deserializzazione se non si è ricevuto l'ack
                    sscanf(serialized_data, "%s %d", &sender, &total);

                    //ricezione timestamp dal server
                    check = recv(conn_socket, (void*)&serialized_data_length, sizeof(uint16_t), 0);
                    serialized_data_len = ntohs(serialized_data_length);
                    check = recv(conn_socket, (void*)timestamp, serialized_data_len, 0);

                    //display
                    printf(ANSI_COLOR_GREEN "    -> Da %s: \n" ANSI_COLOR_RESET, sender);
                    printf("        -> %d messaggi\n", total);
                    printf("        -> timestamp del più recente: %s\n", timestamp);

                    counter++;
                }

                if(counter == 0){
                    printf("    -> Nessun messaggio.\n");
                }

                printf("\n");
            }

            //CONTACTS -> non serve contattare il server
            if(strcmp(users_operation.command, "contacts") == 0){
                /*
                    CONTACTS (operazione aggiuntiva)

                    Mostra a video i contatti registrati nella rubrica dell'utente loggato.
                */
                char contacts_path[BUFFER_SIZE];
                FILE *contacts;
                char contacts_read[CREDENTIALS_LENGTH];
                int contacts_counter = 0;

                sprintf(contacts_path, "%s%s%s", "DEVICES/", login_data.username, "/contacts.bin");

                contacts = fopen(contacts_path, "rb");

                while(fread(&contacts_read, sizeof(contacts_read), 1, contacts) == 1){
                    contacts_counter++;     //conteggio contatti registrati
                }
                
                printf("\n");
                printf(ANSI_COLOR_GREEN "> Hai %d contatti registrati nella rubrica:\n" ANSI_COLOR_RESET, contacts_counter);

                if(contacts_counter != 0){
                    rewind(contacts);

                    while(fread(&contacts_read, sizeof(contacts_read), 1, contacts) == 1){
                        printf("    -> %s\n", contacts_read);       //display dei contatti
                    }
                    
                    fclose(contacts);
                }

                printf("\n");
            }

            //ADD username
            if(strcmp(users_operation.command, "add") == 0){
                /*
                    ADD target

                    Registra target nella rubrica, ovvero in DEVICES/login_data.username/contacts.bin
                */

                int already_added = 0;                   //controllo se il target è già registrato in rubrica
                char contacts_path[] = "DEVICES/";
                char contacts_read[CREDENTIALS_LENGTH];     //stringa per la lettura degli username dalla rubrica
                FILE *contacts;
                strcat(contacts_path, login_data.username);     //composizione del percorso del file contacts.bin personale
                strcat(contacts_path, "/contacts.bin");

                contacts = fopen(contacts_path, "ab+");     //aperto in append e read in binario

                while(fread(&contacts_read, sizeof(contacts_read), 1, contacts) == 1){
                    if(strcmp(contacts_read, users_operation.parameter) == 0){
                        already_added = 1;
                    }
                }

                if(!already_added){
                    char to_be_written[CREDENTIALS_LENGTH];        //stringa che verrà scritta nel file

                    strcpy(to_be_written, users_operation.parameter);

                    rewind(contacts);
                    fwrite(&to_be_written, sizeof(to_be_written), 1, contacts);
                    fclose(contacts);

                    printf(ANSI_COLOR_GREEN "> '%s' è stato registrato in rubrica.\n" ANSI_COLOR_RESET, users_operation.parameter);
                }else{
                    fclose(contacts);

                    printf(ANSI_COLOR_RED "> '%s' è già registrato nella rubrica.\n" ANSI_COLOR_RESET, users_operation.parameter);
                }

                printf("\n");
            }

            //CHAT -> serve contattare il server
            if(strcmp(users_operation.command, "chat") == 0){
                uint16_t target_ok;                 //discriminante per target valido o meno

                check = recv(conn_socket, &target_ok, sizeof(uint16_t), 0);     //ricezione della risposta relativa al controllo del target specificato
                manage_error(check, "Receiving outcome on choice of target username");

                target_ok = ntohs(target_ok);
                if(target_ok){                  //se il target è un contatto esistente, ora va controllato che sia in rubrica
                    
                    uint8_t target_is_friend = is_friend(login_data.username, users_operation.parameter);
                    
                    //se il target non è amico il server non deve entrare nel while di ascolto
                    check = send(conn_socket, &target_is_friend, sizeof(uint8_t), 0);
                    manage_error(check, "Sending if target is friend or not to server");

                    if(target_is_friend){       //arrivati qui il target specificato è valido
                        //dati del target
                        uint16_t target_port;               //porta del dest. se online, 0 SE OFFLINE
                        int target_socket;

                        //variabili per l'invio
                        int chat_socket;
                        char chat_input[BUFFER_SIZE];
                        char to_split[BUFFER_SIZE];
                        char chat_command[BUFFER_SIZE];
                        char chat_parameter[BUFFER_SIZE];
                        
                        check = recv(conn_socket, &target_port, sizeof(uint16_t), 0);       //ricezione porta del target o 0
                        manage_error(check, "Receiving target port number if online, 0 if offline");

                        target_port = ntohs(target_port);           //conversione valore di porta ricevuto dal server

                        system("clear");            //si passa alla 'schermata' della chat

                        if(target_port != 0){
                            printf(ANSI_COLOR_GREEN "%s raggiungibile sulla porta %d\n" ANSI_COLOR_RESET, users_operation.parameter, target_port);
                            printf(ANSI_COLOR_GREEN "Tutte le operazioni sono abilitate.\n" ANSI_COLOR_RESET);
                        }else{
                            printf(ANSI_COLOR_RED "%s è offline.\n" ANSI_COLOR_RESET, users_operation.parameter);
                            printf(ANSI_COLOR_GREEN "E' possibile lo stesso inviare messaggi a %s. Potrà visualizzarli quando tornerà online.\n" ANSI_COLOR_RESET, users_operation.parameter);
                            printf(ANSI_COLOR_GREEN "Non narà possibile condividere file con %s.\n" ANSI_COLOR_RESET, users_operation.parameter);
                        }

                        printf("\n");
                        printf(ANSI_COLOR_GREEN "Legenda:\n" ANSI_COLOR_RESET);
                        printf("    -> *: messaggio non ancora recapitato\n");
                        printf("    -> **: messaggio recapitato\n");
                        printf("\n");

                        printf("\n");
                        printf(ANSI_COLOR_GREEN "Comandi disponibili:\n" ANSI_COLOR_RESET);
                        printf("    -> \\u + INVIO: mostra i contatti della rubrica che sono online\n");
                        printf("    -> \\a username + INVIO: aggiunge il contatto username alla chat\n");
                        printf("    -> \\share filename + INVIO: condivide il file con l'/gli utente/i della chat\n");
                        printf("    -> \\q + INVIO: esce dalla chat e torna al menu iniziale\n");

                        printf("\n");

                        printf("\n");
                        printf(ANSI_COLOR_GREEN "CHAT CON %s:\n" ANSI_COLOR_RESET, users_operation.parameter);
                        printf("\n");

                        printf("\n");

                        //comunicazione se target ha visualizzato o no i msg arrivati dall'iniziatore della chat mentre era offline
                        uint16_t msg_shown;     

                        check = recv(conn_socket, &msg_shown, sizeof(uint16_t), 0);     //ricezione della risposta relativa al controllo del target specificato
                        manage_error(check, "Receiving wheter target executed show with my name as argument or not");
                        msg_shown = ntohs(msg_shown);

                        if(msg_shown){
                            printf(ANSI_COLOR_GREEN "-> %s ha ricevuto tutti i tuoi messaggi.\n" ANSI_COLOR_RESET, users_operation.parameter);
                        }else{
                            printf(ANSI_COLOR_GREEN "-> %s non ha ancora visualizzato gli eventuali messaggi inviati mentre era offline" ANSI_COLOR_RESET, users_operation.parameter);
                        }

                        printf("\n");

                        //DISPLAY DEI VECCHI MESSAGGI SCAMBIATI CON users_operation.parameter
                        display_old_messages(users_operation.parameter, login_data.username);

                        /*
                            registrazione di username|porta di con chi si sta chattando (I contatto della chat)

                            ciclo while:
                                input

                                se:
                                -> \q
                                    break

                                -> \u
                                    mostra username utenti online dalla rubrica -> SERVE AZIONE SERVER

                                    -> \a username (dopo \u)
                                        aggiunge username alla chat
                                        -> richiede porta al server -> SERVE AZIONE SERVER
                                        -> registra record in current_chat.bin

                                    -> \b (dopo \u)
                                        torna alla chat in corso

                                -> share filename
                                    condivide con i partecipanti il file filename


                                -> altrimenti:
                                    a ogni msg si chiama la f per l'invio
                                    invio msg a tutti gli utenti registrati in current_chat.bin
                        */

                        //SOLO SE E' ONLINE: registrazione del target (= parametro del comando chat parameter) nel file degli utenti con cui si sta chattando
                        if(target_port != 0){
                            /*
                                Si registra solo se è online perchè funzioni tipo share file scorrono la lista degli utenti
                                appartenenti alla chat e inviano a tutti, se target è offlibne non deve essere incluso
                                nell'operazione.
                                Anche broadcast_msg() funziona analogamente.
                            */
                            add_to_chat(login_data.username, users_operation.parameter, target_port);
                        }

                        //creazione socket di invio al target
                        chat_socket = socket(AF_INET, SOCK_DGRAM, 0);
                        if(chat_socket < 0){
                            perror("Errore: ");
                            exit(0);
                        }

                        struct online_contacts_data chat_starter;
                        sprintf(chat_starter.username, "%s", login_data.username);
                        chat_starter.port = device_port;

                        //ascolto di input utente e invio messaggi
                        while(TRUE){
                            memset(&chat_input, 0, sizeof(chat_input));     //azzeramento input utente
                            memset(&to_split, 0, sizeof(to_split));
                            memset(&chat_command, 0, sizeof(chat_command));
                            memset(&chat_parameter, 0, sizeof(chat_parameter));

                            fgets(chat_input, BUFFER_SIZE, stdin);      //input

                            //scomposizione dell'input in prima stringa e seconda stringa
                            char *splitter;
                            int counter = 0;
                            int j;
                            char split_key[] = " ";         //divisore dell'input utente

                            strcpy(to_split, chat_input);   //copia dell'input nella stringa da dividere (chat input si lascia intatto)

                            index_command = strcspn(to_split, split_key);

                            for(j = 0; j < index_command; j++){         //copia fino a index_command - 1, si prende il comando eventuale
                                chat_command[j] = to_split[j];
                            }
                            
                            //comandi seguiti da un parametro
                            if(strcmp(chat_command, "share") == 0){
                                splitter = strtok(to_split, " ");        //divide la stringa nelle parole inserite
                                while(splitter != NULL){               //finchè non si arriva a fine stringa
                                    switch(counter){
                                        case 0:
                                            counter++;              //il comando è già stato recuperato prima
                                            break;
                                        case 1:
                                            strcpy(chat_parameter, splitter);        //recupero del parametro
                                            break;
                                    }
                                    splitter = strtok(NULL, " ");
                                }
                            }

                            printf("\x1b[1F"); // Move to beginning of previous line
                            printf("\x1b[2K"); // Clear entire line
                            
                            //GESTIONE AZIONI POSSIBILI
                            if(strcmp(chat_command, "\\q\n") == 0){
                                uint8_t quit_code = 3;

                                //si invia il comando al server
                                check = send(conn_socket, &quit_code, sizeof(uint8_t), 0);
                                manage_error(check, "Sending usernames command to server");

                                //chiamata alla funzione apposita
                                quit_current_chat(chat_socket, login_data);

                                break;
                            }else if(strcmp(chat_command, "\\u\n") == 0){
                                /*
                                    Per ogni username della rubrica, contatta il server
                                    per sapere se è online al momento oppure no.

                                    Mostra solo i contatti online, ovvero quelli inseribili nella chat.
                                */

                                uint8_t usernames_code = 2;         //codice che indica l'input del comando \u al server

                                if(get_server_status(login_data.username)){
                                    //si invia il comando al server
                                    check = send(conn_socket, &usernames_code, sizeof(uint8_t), 0);
                                    manage_error(check, "Sending usernames command to server");

                                    FILE *contacts;
                                    char contacts_path[BUFFER_SIZE];
                                    char contacts_read[CREDENTIALS_LENGTH];
                                    uint16_t contacts_counter = 0;
                                    uint16_t recvd_port;
                                    uint16_t msg_length;
                                    int msg_len;
                                    char *end_msg = "END";                      //messaggio di ack per il server

                                    struct online_contacts_data now_online[MAX_CONTACTS];       //struttura per il salvataggio dei dati dei contatti online al momento
                                    int i = 0;

                                    sprintf(contacts_path, "%s%s%s", "DEVICES/", login_data.username, "/contacts.bin");

                                    contacts = fopen(contacts_path, "rb");          //file rubrica

                                    printf(ANSI_COLOR_GREEN "> Contatti online al momento: \n");
                                    while(fread(&contacts_read, sizeof(contacts_read), 1, contacts) == 1){
                                        //per ogni contatto della rubrica invia l'username al server
                                        //riceve la porta su cui è raggiungibile se online, 0 altrimenti
                                        msg_len = strlen(contacts_read);
                                        msg_length = htons(msg_len);            //invio dimensione messaggio
                                        check = send(conn_socket, (void*)&msg_length, sizeof(uint16_t), 0);
                                        manage_error(check, "Sending contact length to server");

                                        check = send(conn_socket, (void*)contacts_read, msg_len, 0);        //invio contatto
                                        manage_error(check, "Sending contact to server");

                                        check = recv(conn_socket, &recvd_port, sizeof(uint16_t), 0);    //ricezione porta, 0 se offline
                                        manage_error(check, "Receiving contact port");

                                        recvd_port = ntohs(recvd_port);
                                        if(recvd_port != 0){                //se la porta è 0 il contatto è offline e non si mostra
                                            printf(ANSI_COLOR_RESET "    -> %s, porta: %d\n", contacts_read, recvd_port);
                                            contacts_counter++;

                                            strcpy(now_online[i].username, contacts_read);      //salvataggio dei soli contatti online nell'array di struct
                                            now_online[i].port = recvd_port;
                                            i++;
                                        }
                                    }

                                    //invio ACK di fine invio al server, che può così uscire dal ciclo e aspettare altri comandi dalla chat
                                    msg_len = strlen(end_msg);
                                    msg_length = htons(msg_len);
                                    check = send(conn_socket, (void*)&msg_length, sizeof(uint16_t), 0);
                                    manage_error(check, "Sending END ack msg length to server");
                                    check = send(conn_socket, (void*)end_msg, msg_len, 0);
                                    manage_error(check, "Sending END ack msg to server");

                                    fclose(contacts);

                                    //se non ci sono amici online, si mostra a video, altrimenti si mostrano le ozioni disponibili
                                    if(contacts_counter == 0){
                                        printf("%d\n" ANSI_COLOR_RESET, contacts_counter);
                                    }else{
                                        printf(ANSI_COLOR_GREEN "Comandi disponibili: \n");
                                        printf("    -> \\a username: aggiunge l'utente 'username' alla chat\n");
                                        printf("    -> \\b: torna alla chat\n" ANSI_COLOR_RESET);

                                        //a questo punto l'array now_online contiene username e porta dei contatti online

                                        char add_input[BUFFER_SIZE];
                                        char add_command[BUFFER_SIZE];
                                        char add_parameter[BUFFER_SIZE];
                                        
                                        do{
                                            memset(&add_input, 0, sizeof(add_input));
                                            memset(&add_command, 0, sizeof(add_command));
                                            memset(&add_parameter, 0, sizeof(add_parameter));

                                            printf("Digitare un comando valido: ");

                                            fgets(add_input, BUFFER_SIZE, stdin);      //input comando

                                            //scomposizione dell'input in prima stringa e seconda stringa
                                            counter = 0;
                                            char split_key[]= " ";         //divisore dell'input utente

                                            index_command = strcspn(add_input, split_key);

                                            for(j = 0; j < index_command; j++){         //copia fino a index_command - 1, si prende il comando eventuale
                                                add_command[j] = add_input[j];
                                            }
                                            
                                            //comandi seguiti da un parametro
                                            if(strcmp(add_command, "\\a") == 0){
                                                splitter = strtok(add_input, " ");        //divide la stringa nelle parole inserite
                                                while(splitter != NULL){               //finchè non si arriva a fine stringa
                                                    switch(counter){
                                                        case 0:
                                                            counter++;              //il comando è già stato recuperato prima
                                                            break;
                                                        case 1:
                                                            strcpy(add_parameter, splitter);        //recupero del parametro
                                                            break;
                                                    }
                                                    splitter = strtok(NULL, " ");
                                                }
                                            }
                                        }while(strcmp(add_command, "\\a") != 0 && strcmp(add_command, "\\b\n") != 0);

                                        if(strcmp(add_command, "\\a") == 0){
                                            add_parameter[strlen(add_parameter) - 1] = '\0';
                                            
                                            int j;
                                            for(j = 0; j < i; j++){
                                                if(strcmp(now_online[j].username, add_parameter) == 0){
                                                    add_to_chat(login_data.username, now_online[j].username, now_online[j].port);
                                                    printf(ANSI_COLOR_GREEN "> %s ha aggiunto %s alla chat.\n" ANSI_COLOR_RESET, login_data.username, now_online[j].username);

                                                    broadcast_msg("ADD", chat_input, login_data.username, chat_socket, now_online[j], 0);

                                                    break;
                                                }
                                            }
                                        }
                                    }

                                    printf("\n");
                                }else{                  
                                    printf(ANSI_COLOR_RED "> Server offline. Questa funzione non è al momento disponibile.\n" ANSI_COLOR_RESET);
                                    printf("\n");
                                }
                            }else if(strcmp(chat_command, "share") == 0){
                                /*
                                    Si condivide il file chat_parameter agli utenti che partecipano alla chat in corso
                                */

                                //filename --> dentro chat_parameter
                                FILE *to_be_shared;

                                chat_parameter[strlen(chat_parameter) - 1] = '\0';

                                to_be_shared = fopen(chat_parameter, "rb");     //apertura file in BINARIO

                                if(to_be_shared){          //il file esiste
                                    fclose(to_be_shared);
                                    
                                    char chunk[CHUNK_DIM];
                                    struct online_contacts_data null_user;    //per completare gli argomenti in broadcast_msg
                                    FILE *current_chat;
                                    struct current_chat_entry current_chat_read;
                                    char current_chat_path[BUFFER_SIZE];

                                    struct sockaddr_in target_addr;       //indirizzo a cui inviare il file
                                    int share_socket;
                                    uint16_t chunk_length;   //dimensione chunk da inviare
                                    int chunk_len;

                                    sprintf(current_chat_path, "%s%s%s", "DEVICES/", login_data.username, "/current_chat.bin");

                                    current_chat = fopen(current_chat_path, "rb");      //per leggere i destinatari del file da condividere

                                    fseek(current_chat, 0, SEEK_END);       //controllo se il file current_chat.bin contiene partecipanti a cui inviare il file
                                    int file_size = ftell(current_chat);

                                    if(file_size != 0){     //ci sono utenti a cui inviare il file
                                        rewind(current_chat);

                                        to_be_shared = fopen(chat_parameter, "rb");     //apertura file da inviare

                                        //invio del nome del file
                                        broadcast_msg("FILENAME", chat_parameter, login_data.username, chat_socket, null_user, 0);     //invio del nome del file

                                        //conteggio chunk INTERI da inviare
                                        int tot_chunks = 0;
                                        while(fread(&chunk, sizeof(chunk), 1, to_be_shared) == 1){
                                            tot_chunks++;
                                        }
                                        rewind(to_be_shared);

                                        //per ogni utente con cui si chatta
                                        //si invia una copia del file per volta, diviso in chunks di CHUNK_DIM
                                        while(fread(&current_chat_read, sizeof(struct current_chat_entry), 1, current_chat) == 1){
                                            rewind(to_be_shared);       //riporta il puntatore all'inizio del file per una nuova condivisione
                                            share_socket = socket(AF_INET, SOCK_STREAM, 0);     //socket su cui si invia il file
                                            
                                            //creazione indirizzo target a cui inviare il file
                                            memset(&target_addr, 0, sizeof(target_addr));
                                            target_addr.sin_family = AF_INET;
                                            target_addr.sin_port = htons(current_chat_read.port);           //porta letta dal file
                                            inet_pton(AF_INET, "127.0.0.1", &target_addr.sin_addr);

                                            check = connect(share_socket, (struct sockaddr*)&target_addr, sizeof(target_addr));     //connessione

                                            //tutti i chunk interi si inviano qui
                                            int i;
                                            for(i = 0; i < tot_chunks; i++){
                                                fread(&chunk, sizeof(chunk), 1, to_be_shared);  //lettura dei chunk di dimensione CHUNK_DIM
                                                chunk_len = strlen(chunk);
                                                chunk_length = htons(chunk_len);    

                                                //invio lunghezza
                                                check = send(share_socket, (void*)&chunk_length, sizeof(uint16_t), 0);
                                                manage_error(check, "Sending chunk length to target device");

                                                

                                                //invio chunk
                                                check = send(share_socket, (void*)chunk, chunk_len, 0);
                                                manage_error(check, "Sending chunk to target device");

                                                sleep(CHUNK_RATE);     //intervallo tra un invio e l'altro
                                            }

                                            //il puntatore nel file è nella posizione di inizio dell'ultimo chunk da inviare
                                            memset(&chunk, 0, sizeof(chunk));
                                            for(i = 0; i < CHUNK_DIM; i++){
                                                //i caratteri da leggere saranno sempre in numero minore di CHUNK_DIM, l'operazione è veloce     
                                                check = fscanf(to_be_shared, "%c", &chunk[i]);  //si riempie il chunk con i restanti caratteri
                                                
                                                if(check == EOF){   //a fine file si esce
                                                    break;
                                                }
                                            }
                                            
                                            //invio dell'ultimo chunk rimasto, di lunghezza minore di CHUNK_DIM
                                            chunk_len = strlen(chunk);
                                            chunk_length = htons(chunk_len);
                                            check = send(share_socket, (void*)&chunk_length, sizeof(uint16_t), 0);
                                            manage_error(check, "Sending chunk length to target device");
                                            check = send(share_socket, (void*)chunk, chunk_len, 0);
                                            manage_error(check, "Sending chunk to target device");
                                            

                                            //invio ACK di fine trasmissione, serve al destinatario per uscire dal while(TRUE)
                                            memset(&chunk, 0, sizeof(chunk));
                                            sprintf(chunk, "%s", "END_OF_FILE");   
                                            chunk_len = strlen(chunk);
                                            chunk_length = htons(chunk_len);
                                            check = send(share_socket, (void*)&chunk_length, sizeof(uint16_t), 0);
                                            check = send(share_socket, (void*)chunk, chunk_len, 0);

                                            close(share_socket);  //chiusura socket dedicato          
                                        
                                            printf(ANSI_COLOR_GREEN "Hai condiviso il file '%s' con %s.\n" ANSI_COLOR_RESET, chat_parameter, current_chat_read.username);
                                        }

                                        fclose(to_be_shared);
                                    }else{
                                        printf(ANSI_COLOR_RED "Nessun utente a cui inviare il file...\n" ANSI_COLOR_RESET);
                                    }
                                }else{                                          //il file non esiste, msg di errore e torna all'inizio
                                    printf(ANSI_COLOR_RED "Nessun file '%s' nella current folder.\n"ANSI_COLOR_RESET, chat_parameter);
                                }

                            }else{
                                //MESSAGGIO DA INVIARE IN CHAT, SI INVIA A TUTTI I COMPONENTI DI current_chat.bin
                                /*
                                    SE TARGET DEVICE E' ONLINE:
                                        si manda il msg a lui e agli altri eventuali partecipanti della chat (tutto tramite broadcast_msg())
                                        SE è l'unico nella chat si salvano i msg che gli si inviano
                                        ALTRIMENTI non si salva lo storico dei msg delle chat di gruppo
                                */
                                uint8_t save_this_msg = 0;
                                
                                if(target_port != 0){       //TARGET DEVICE ONLINE     
                                    printf("> Tu**: %s\n", chat_input);         //msg sicuramente consegnato a tutti -> **
                                    
                                    //conta di quanti membri sono presenti nella chat di gruppo
                                    FILE *current_chat;
                                    char current_chat_path[BUFFER_SIZE];
                                    int chat_members = 0;
                                    struct current_chat_entry current_chat_read;

                                    sprintf(current_chat_path, "%s%s%s", "DEVICES/", login_data.username, "/current_chat.bin");

                                    current_chat = fopen(current_chat_path, "rb");
                                    while(fread(&current_chat_read, sizeof(current_chat_read), 1, current_chat) == 1){
                                        chat_members++;
                                    }
                                    fclose(current_chat); 

                                    //SALVATAGGIO SU FILE SOLO SE LA CHAT E' SINGOLA (CON UN SOLO UTENTE DEST)
                                    if(chat_members == 1){
                                        struct chat_elem_entry chat_elem;

                                        strcpy(chat_elem.sender, login_data.username);
                                        strcpy(chat_elem.recipient, users_operation.parameter);
                                        strcpy(chat_elem.msg, chat_input); 
                                        strcpy(chat_elem.timestamp, "");
                                        chat_elem.delivered = 1;

                                        save_to_log(chat_elem, login_data.username);        //salvataggio tramite funzione apposita
                                    
                                        save_this_msg = 1;  //CHI RICEVE IL MSG LO DEVE SALVARE SUL SUO FILE
                                    }
                                }else{          //TARGET DEVICE OFFLINE
                                    /*
                                        Invio dei messaggi al server, che li memorizza.
                                        Si inviano record del tipo:
                                            mitt dest msg timestamp_invio delivered_si_o_no

                                        Qui il campo delivered vale 0 (il msg non è consegnato al dest)
                                    */

                                    //conta di quanti membri sono presenti nella chat di gruppo
                                    FILE *current_chat;
                                    char current_chat_path[BUFFER_SIZE];
                                    int chat_members = 0;
                                    struct current_chat_entry current_chat_read;

                                    sprintf(current_chat_path, "%s%s%s", "DEVICES/", login_data.username, "/current_chat.bin");

                                    current_chat = fopen(current_chat_path, "rb");
                                    while(fread(&current_chat_read, sizeof(current_chat_read), 1, current_chat) == 1){
                                        chat_members++;
                                    }
                                    fclose(current_chat);
                                    

                                    //SALVATAGGIO SU FILE SOLO SE LA CHAT E' SINGOLA (CON UN SOLO UTENTE DEST)
                                    if(chat_members == 0){      //si sta chattando solo con target, nessun altro è stato agguinto, allora il msg va memorizzato

                                        printf("> Tu*: %s\n", chat_input);
                                    
                                        uint8_t chat_target_offline_code = 1;         //codice del comando per il server
                                        char server_msg[BUFFER_SIZE];      //record per il server
                                        uint16_t server_msg_length;        //lunghezza messaggio per il server
                                        int server_msg_len;

                                        //si invia il comando al server
                                        check = send(conn_socket, &chat_target_offline_code, sizeof(uint8_t), 0);
                                        manage_error(check, "Sending chat_with_target_offline command to server");

                                        //copia valori nella struct del record
                                        struct chat_elem_entry chat_elem;
                                        strcpy(chat_elem.sender, login_data.username);
                                        strcpy(chat_elem.recipient, users_operation.parameter);
                                        strcpy(chat_elem.msg, chat_input);
                                        chat_elem.delivered = 0;        //il msg non è ancora recapitato chiaramente
                                        
                                        save_to_log(chat_elem, login_data.username);         //salvataggio nel log dell'utente

                                        //invio del mittente e del destinatario al server (il mittente nemmeno lo manderei ma di là fa macello non so perchè e sinceramente non ce la faccio proprio più)
                                        memset(&server_msg, 0, sizeof(server_msg));
                                        strcpy(server_msg, chat_elem.sender);
                                        server_msg_len = strlen(server_msg);      //calcolo lunghezza
                                        server_msg_length = htons(server_msg_len);
                                        check = send(conn_socket, (void*)&server_msg_length, sizeof(uint16_t), 0);
                                        check = send(conn_socket, (void*)server_msg, server_msg_len, 0);

                                        memset(&server_msg, 0, sizeof(server_msg));
                                        strcpy(server_msg, chat_elem.recipient);
                                        server_msg_len = strlen(server_msg);      //calcolo lunghezza
                                        server_msg_length = htons(server_msg_len);
                                        check = send(conn_socket, (void*)&server_msg_length, sizeof(uint16_t), 0);
                                        check = send(conn_socket, (void*)server_msg, server_msg_len, 0);

                                        //invio del messaggio al server
                                        memset(&server_msg, 0, sizeof(server_msg));
                                        chat_elem.msg[strlen(chat_elem.msg) - 1] = '\0';
                                        strcpy(server_msg, chat_elem.msg);    //copia valore
                                        server_msg_len = strlen(server_msg);      //calcolo lunghezza
                                        server_msg_length = htons(server_msg_len);
                                        check = send(conn_socket, (void*)&server_msg_length, sizeof(uint16_t), 0);
                                        check = send(conn_socket, (void*)server_msg, server_msg_len, 0);

                                    }else{          //ci sono anche altri utenti nella chat di gruppo        
                                        printf("> Tu: %s\n", chat_input);       //non si mostra * o ** visto che non si implementa la storia delle chat di gruppo
                                    }
                                }

                                //INVIO MESSAGGIO A TUTTI I MEMBRI
                                struct online_contacts_data null_user;    //per completare gli argomenti, non serve
                                broadcast_msg("CHAT", chat_input, login_data.username, chat_socket, null_user, save_this_msg);
                            }
                        }
                    }else{              //username non registrato in rubrica
                        printf("\n");
                        printf(ANSI_COLOR_RED "> %s non è registrato nella rubrica. Per poter chattare con %s, deve prima essere registrato.\n" ANSI_COLOR_RESET, users_operation.parameter, users_operation.parameter);
                    }
                }else{              //username inesistente nel sistema
                    printf("\n");
                    printf(ANSI_COLOR_RED "> Nessun utente con username '%s' registrato nel sistema.\n" ANSI_COLOR_RESET, users_operation.parameter);
                }
            }

            //OUT -> serve contattare il server
            if(strcmp(users_operation.command, "out") == 0){
                char current_chat_path[BUFFER_SIZE];
            
                sprintf(current_chat_path, "%s%s%s", "DEVICES/", login_data.username, "/current_chat.bin");
                fclose(fopen(current_chat_path, "wb"));         //si svuota il file 'current_chat.bin'

                printf("\n");
                printf(ANSI_COLOR_GREEN "> Hai eseguito il logout.\n" ANSI_COLOR_RESET);

                system("tput init");
                close(conn_socket);
                exit(0);                //terminazione processo
            }

        }while(strcmp(users_operation.command, "out") != 0);
    }else{
        /*
            RICEZIONE MSG DA QUALSIASI DEVICE

            Ad ogni msg ricevuto si controlla il mittente.
            Se compare in current_chat.bin allora è un msg della chat attiva,
            altrimenti si tratta come notifica.

            Se è un messaggio dal server, è un ACK di disconnessione.
        */

        //VARIABILI PER LA CHAT
        char recvd_recipient[CREDENTIALS_LENGTH];
        char recvd_serialized[BUFFER_SIZE];
        struct recvd_data{
            char msg_type[BUFFER_SIZE];         //più tipi di parametro a seconda di cosa viene ricevuto
            char param1[BUFFER_SIZE];
            uint16_t param2;
        }recvd_data;
        int target_addr_len;
        int chat_socket;
        struct sockaddr_in device_addr;
        struct sockaddr_in target_addr;

        //VARIABILI PER IL FILE SHARING
        int share_socket;


        target_addr_len = sizeof(target_addr);

        chat_socket = socket(AF_INET, SOCK_DGRAM, 0);      //socket per op. di chat, add, quit, server ack

        share_socket = socket(AF_INET, SOCK_STREAM, 0);    //socket usato solo per il file sharing


        //creazione indirizzo device ricevente, con i parametri suoi
        memset(&device_addr, 0, sizeof(device_addr));
        device_addr.sin_family = AF_INET;
        device_addr.sin_port = htons(device_port);         //porta del DEVICE RICEVENTE        
        device_addr.sin_addr.s_addr = INADDR_ANY;           //ASCOLTO DA TUTTI

        check = bind(chat_socket, (struct sockaddr*)&device_addr, sizeof(device_addr));     //associazione ai socket usati

        inet_pton(AF_INET, "127.0.0.1", &device_addr.sin_addr);
        check = bind(share_socket, (struct sockaddr*)&device_addr, sizeof(device_addr));
        check = listen(share_socket, 10);


        //ciclo di ASCOLTO, si usa sempre lo stesso socket di ascolto chat_socket
        while(TRUE){
            memset(&recvd_recipient, 0, sizeof(recvd_recipient));
            memset(&recvd_serialized, 0, sizeof(recvd_serialized));
            memset(&recvd_data, 0, sizeof(recvd_data));

            //ricezione username mittente 
            msg_len = recvfrom(chat_socket, recvd_recipient, CREDENTIALS_LENGTH, 0, (struct sockaddr*)&target_addr, &target_addr_len);
            
            //CONTROLLO SE SI TRATTA DI UN ACK DAL SERVER O DI UN MSG DA UN UTENTE
            if(strcmp(recvd_recipient, "SERVER_ACK") == 0){
                //MESSAGGIO DI DISCONNESSIONE DEL SERVER

                while(TRUE){    //UDP inaffidabile, il server manda DISCONNECTION_ACKS (definito in serv.c) pkt
                    msg_len = recvfrom(chat_socket, recvd_serialized, GENERIC_MSG_LENGTH, 0, (struct sockaddr*)&target_addr, &target_addr_len);
                    break;      //basta che ne arrivi 1 per capire che il server è disconnesso
                }

                update_server_status(0, login_data.username);       //aggiornamento stato server
                
                printf("\n");
                printf(ANSI_COLOR_RED "> SERVER DISCONNESSO.\n" ANSI_COLOR_RESET);   

            }else{
                //MESSAGGIO DA UN UTENTE

                //ricezione messaggio
                msg_len = recvfrom(chat_socket, recvd_serialized, BUFFER_SIZE, 0, (struct sockaddr*)&target_addr, &target_addr_len);
                
                sscanf(recvd_serialized, "%s", &recvd_data.msg_type);       //recupera il TIPO di msg ricevuto

                //a seconda del tipo di msg ricevuto si agisce di conseguenza
                if(strcmp(recvd_data.msg_type, "CHAT") == 0){
                    char chat_msg[BUFFER_SIZE];     //msg vero e proprio, da visualizzare
                    uint8_t save_this_msg;          //il mittente dice se il msg ricevuto deve essere salvato nel log

                    //recupero del parametro per il salvataggio o meno del msg
                    sscanf(recvd_serialized, "%s %d", &recvd_data.msg_type, &save_this_msg);

                    memset(&chat_msg, 0, sizeof(chat_msg));

                    msg_len = recvfrom(chat_socket, chat_msg, BUFFER_SIZE, 0, (struct sockaddr*)&target_addr, &target_addr_len);
 
                    //controllo se chi ha inviato il msg ricevuto è l'/uno degli utente/i con cui si sta chattando
                    int chatting = check_if_chatting(login_data.username, recvd_recipient);

                    if(chatting){           //se sì, display del msg come nella chat
                        printf("> %s**: %s\n", recvd_recipient, chat_msg);
                    }else{                  //se no, display del msg stile notifica
                        printf("\n");
                        printf(ANSI_COLOR_GREEN "> 1 NUOVO MESSAGGIO DA %s: " ANSI_COLOR_RESET, recvd_recipient);
                        printf("%s\n", chat_msg);
                    }

                    //messaggio da salvare
                    if(save_this_msg == 1){
                        struct chat_elem_entry to_be_saved;
                        to_be_saved.delivered = 1;
                        strcpy(to_be_saved.msg, chat_msg);
                        strcpy(to_be_saved.sender, recvd_recipient);
                        strcpy(to_be_saved.recipient, login_data.username);
                        strcpy(to_be_saved.timestamp, "");

                        save_to_log(to_be_saved, login_data.username);          //salvataggio nel log
                    }
                }else if(strcmp(recvd_data.msg_type, "ADD") == 0){
                    sscanf(recvd_serialized, "%s %s %d", &recvd_data.msg_type, &recvd_data.param1, &recvd_data.param2);       //recupera gli altri dati serializzati

                    //recvd_data.param1 ---> username di chi è stato aggiunto alla chat
                    //recvd_data.param2 ---> porta di chi è stato aggiunto alla chat

                    //display della notifica dell'aggiunta ad una chat all'utente che è stato aggiunto
                    if(strcmp(recvd_data.param1, login_data.username) == 0){
                        printf(ANSI_COLOR_GREEN "> Sei stato aggiunto da %s alla sua chat.\n" ANSI_COLOR_RESET, recvd_recipient);
                        printf(ANSI_COLOR_GREEN "> Riceverai i messaggi che %s invia sulla sua chat.\n" ANSI_COLOR_RESET, recvd_recipient);
                    }

                    //gli altri utenti che ricevono questo pkt non lo considerano
                }else if(strcmp(recvd_data.msg_type, "FILENAME") == 0){
                    sscanf(recvd_serialized, "%s %s", &recvd_data.msg_type, &recvd_data.param1);

                    //recvd_data.param1 ---> nome del file che si riceve

                    int len = sizeof(device_addr);
                    int transf_socket = accept(share_socket, (struct sockaddr*)&device_addr, &len);

                    FILE *shared_file;
                    char recvd_chunk[CHUNK_DIM];        //chunk ricevuto di volta in volta, viene appeso al file
                    char dest_path[BUFFER_SIZE];
                    uint16_t chunk_length;              //lunghezza chunk ricevuto
                    int chunk_len;

                    sprintf(dest_path, "%s%s%s%s", "DEVICES/", login_data.username, "/", recvd_data.param1);

                    shared_file = fopen(dest_path, "ab");       //creazione e apertura file in append

                    //ciclo di ricezione del file
                    while(TRUE){
                        memset(&recvd_chunk, 0, sizeof(recvd_chunk));
                        chunk_length = 0;
                        chunk_len = 0;

                        check = recv(transf_socket, (void*)&chunk_length, sizeof(uint16_t), 0);  //ricezione dimensione
                        manage_error(check, "Receiving chunk length");
                        chunk_len = ntohs(chunk_length);        //conversione dimensione
                        check = recv(transf_socket, (void*)recvd_chunk, chunk_len, 0);      //ricezione chunk
                        manage_error(check, "Receiving actual chunk");

                        sleep(CHUNK_RATE);          //sincronizzazione con il mittente dei chunk

                        if(strcmp(recvd_chunk, "END_OF_FILE") != 0){        //se non è il chunk di ACK, si scrive su file
                            fwrite(&recvd_chunk, strlen(recvd_chunk), 1, shared_file);
                        }else{
                            break;
                        }
                    }

                    close(transf_socket);       //arrivato l'ACK, si chiude tutto
                    fclose(shared_file);

                    printf("\n");
                    printf(ANSI_COLOR_GREEN "%s ha condiviso il file '%s'\n" ANSI_COLOR_RESET, recvd_recipient, recvd_data.param1);
                }else if(strcmp(recvd_data.msg_type, "QUIT") == 0){
                    sscanf(recvd_serialized, "%s %s", &recvd_data.msg_type, &recvd_data.param1);

                    //recvd_data.param1 ---> username da rimuovere dal file current_chat.bin

                    int chatting_counter = 0;
                    FILE *current_chat;
                    char current_chat_path[BUFFER_SIZE];
                    struct current_chat_entry current_chat_read;

                    sprintf(current_chat_path, "%s%s%s", "DEVICES/", login_data.username, "/current_chat.bin");

                    current_chat = fopen(current_chat_path, "rb");

                    while(fread(&current_chat_read, sizeof(struct current_chat_entry), 1, current_chat) == 1){
                        chatting_counter++;
                    }

                    //se si chattava con una persona sola, era una chat singola, i messaggi che si continuano a mandare continuano ad arrivare
                    if(chatting_counter != 1){
                        FILE *current_chat_temp;
                        char current_chat_temp_path[BUFFER_SIZE];
                        
                        rewind(current_chat);

                        sprintf(current_chat_temp_path, "%s%s%s", "DEVICES/", login_data.username, "/current_chat_temp.bin");
                        
                        current_chat_temp = fopen(current_chat_temp_path, "wb");

                        while(fread(&current_chat_read, sizeof(struct current_chat_entry), 1, current_chat) == 1){
                            if(strcmp(current_chat_read.username, recvd_data.param1) != 0){     //si ricopiano gli username diversi da quello che esce dalla chat
                                fwrite(&current_chat_read, sizeof(struct current_chat_entry), 1, current_chat_temp);
                            }
                        }

                        fclose(current_chat);
                        fclose(current_chat_temp);

                        remove(current_chat_path);          //rimozione file non aggiornato e rinomina del file aggiornato
                        rename(current_chat_temp_path, current_chat_path);
                    }
                }
            }
        }
    }

    close(conn_socket);

    return 0;
}