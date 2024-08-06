//###########################################################################
//
// FILE:    	server.c
//
// DESCRIPTION: Funkcie serveru, rozdelenie programu na CHLD procesy, nastavenie
//              server soketu, serverová strana komunikácie a vlákno na ukončenie
//              programu.
//
// AUTHOR:		Marek Sýkorka
//
//###########################################################################

//
// Include
//

// Include špecifický pre program
#include "global_defs.h"
#include "server.h"
#include "clients.h"

//
// Functions
//

// Inicializačné
void initialize_child_processes(PID_INFO *_pid_info)
{
    DEBUG_MESSAGE debug_msg;
    debug_msg.mesg_type = 1;

    // Create Child 1
    if ((_pid_info->pid1 = fork()) < 0)
    {
        _pid_info->pid1 = -1;

        // ERROR creating processes - terminate app
        server_error = errno;
        kill(getpid(),SIGTERMINATE);
    }
    
    // Child 1 obsluha
    if(_pid_info->pid1==0)
        client1_main();
    else
    {
        sprintf(debug_msg.mesg_text, "{SERV} [PROC]  (i)   Child 1 PID:%d CREATED.",_pid_info->pid1);
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    }        
    
    if(_pid_info->pid1 != 0)
    {
        // Create Child 2
        if ((_pid_info->pid2 = fork()) < 0)
        {
            _pid_info->pid2 = -1;

            // ERROR creating processes - terminate app
            server_error = errno;
            kill(getpid(),SIGTERMINATE);
        }
        
        // Child 2 obsluha
        if(_pid_info->pid2==0)
            client2_main();
        else
        {
            sprintf(debug_msg.mesg_text, "{SERV} [PROC]  (i)   Child 2 PID:%d CREATED.",_pid_info->pid2);
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        }
    }

    if(_pid_info->pid1 != 0 && _pid_info->pid2 != 0)
    {
        // Create Child 3
        if ((_pid_info->pid3 = fork()) < 0)
        {
            _pid_info->pid3 = -1;

            // ERROR creating processes - terminate app
            server_error = errno;
            kill(getpid(),SIGTERMINATE);
        }
        
        //Child 3 obsluha
        if(_pid_info->pid3==0)
            client3_main();
        else
        {
            sprintf(debug_msg.mesg_text, "{SERV} [PROC]  (i)   Child 3 PID:%d CREATED.",_pid_info->pid3);
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        }
    }

    if(_pid_info->pid1 != 0 && _pid_info->pid2 != 0 && _pid_info->pid3 != 0)
    {
        // Create Child 4
        if ((_pid_info->pid4 = fork()) < 0)
        {
            _pid_info->pid4 = -1;

            // ERROR creating processes - terminate app
            server_error = errno;
            kill(getpid(),SIGTERMINATE);
        }
        
        //Child 4 obsluha
        if(_pid_info->pid4==0)
            client4_main();
        else
        {
            sprintf(debug_msg.mesg_text, "{SERV} [PROC]  (i)   Child 4 PID:%d CREATED.",_pid_info->pid4);
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        }
    }
}

void initialization_complete(PID_INFO *_pid_info)
{
    sleep(1);
    kill(_pid_info->pid1,SIGINIT);
    sleep(1);
    kill(_pid_info->pid2,SIGINIT);
    sleep(1);
    kill(_pid_info->pid3,SIGINIT);
    sleep(1);
    kill(_pid_info->pid4,SIGINIT);
}

// Sokety a komunikácia
int create_server_socket(DEBUG_MESSAGE debug_msg)
{
    // Vytvorenie IPv4 sieťového socketu
    int sock_decs = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_decs == -1)
    {
        sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Cannot create server socket.");
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        server_error = errno;
        kill(getpid(),SIGTERMINATE);
    }
    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Server socket %d created.", sock_decs);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Nastavenie serverového soketu
    struct sockaddr_in server;  
    memset(&server, 0, sizeof(server));  
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;  
    server.sin_port = htons(SOCK_PORT);

    // Pridelenie sieťovej adresy soketu podľa nastavení v štruktúre sockaddr_in
    if (bind(sock_decs, (struct sockaddr*)&server, sizeof(server)) != 0)
    {
        sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Cannot bind server socket %d.", sock_decs);
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        server_error = errno;
        kill(getpid(),SIGTERMINATE);
    }
    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Server socket %d bound.", sock_decs);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Počúvanie prichádzajúcich požiadaviek o pripojenie na serverovom sokete 
    if (listen(sock_decs, 20) != 0)
    {
        sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Server socket %d cannot listen.", sock_decs);
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        server_error = errno;
        kill(getpid(),SIGTERMINATE);
    }
    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Server socket %d listening.", sock_decs);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    return sock_decs;
}

int client_authentification(DEBUG_MESSAGE debug_msg, SOCKET_INFO* _sockets, int _server_sock)
{
    int client_sock, sock_status;
    SOCKET_DATA sock_msg;

    // Nastavenie nového komunikačného soketu s klientom
    struct sockaddr_in client;  
    memset(&client, 0, sizeof(client));
    socklen_t cl1_len = sizeof(client);

    // Akceptovanie pripojenia
    client_sock = accept(_server_sock, (struct sockaddr*)&client, &cl1_len);  
    if (client_sock == -1)
    {
        sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Cannot accept client connection.");
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        server_error = errno;
        kill(getpid(),SIGTERMINATE);
    }
    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Client connected.");
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Zaslanie Autentifikačného paketu.
    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (>)   Sending AUTH request.");
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    sock_msg.control_word = SOCK_AUTH_REQ;
    if (write(client_sock, &sock_msg, sizeof(sock_msg)) < 0)
    {
        sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   AUTHentification failed - Write error.");
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        server_error = errno;
        kill(getpid(),SIGTERMINATE);
    }

    // Čítanie Autentifikačného paketu
    if (read(client_sock, &sock_msg, sizeof(sock_msg)) < 0)
    {
        sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   AUTHentification failed - Read error.");
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        server_error = errno;
        kill(getpid(),SIGTERMINATE);
    }
    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (<)   AUTH message received.");
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Authentifikácia
    switch(sock_msg.control_word)
    {
        case P_CLI1: 
                    _sockets->client1_sock = client_sock; 
                    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Authentification of Client 1 succesful. Socket:%d",client_sock);
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    break;
        case P_CLI2:
                    _sockets->client2_sock = client_sock;
                    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Authentification of Client 2 succesful. Socket:%d",client_sock);
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    break;
        case P_CLI3:
                    _sockets->client3_sock = client_sock;
                    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Authentification of Client 3 succesful. Socket:%d",client_sock);
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    break;
        case P_CLI4: 
                    _sockets->client4_sock = client_sock; 
                    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Authentification of Client 4 succesful. Socket:%d",client_sock);
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0); 
                    break;
        default:
                    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   AUTHentification failed - Unknown CLIENT.");
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    server_error = EINVAL;
                    kill(getpid(),SIGTERMINATE);
                    break;
    }

    // Yaslanie ACKW potvrdenia o autentifikácii.
    sock_msg.control_word = SOCK_ACKW;
    if (write(client_sock, &sock_msg, sizeof(sock_msg)) < 0)
    {
        sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Write failed, ACKW not send.");
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        server_error = errno;
        kill(getpid(),SIGTERMINATE);
    }
    sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (>)   Sending ACKWnoladge of AUTHentification request.");
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
}

// Hlavná obslužná funkcia serveru
int server_main(SERV_OBJ *_serv_info)
{
    // Lokálne premenné
    DEBUG_MESSAGE debug_msg; // Štruktúra dát pre frontu správ
    debug_msg.mesg_type = 1;
    PID_INFO *_pid_info = &_serv_info->_pid_info; // Štruktúra s PID CHILD procesov
    SOCKET_INFO sockets; // Štruktúra s Soket deskriptormi klientov

    sprintf(debug_msg.mesg_text, "{SERV} [STAT]  (i)   Server main function.");
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Vytvorenie Server soketu
    int serv_sock_decs = create_server_socket(debug_msg);
    _serv_info->server_sock = serv_sock_decs;

    // Vyslanie signálu ktorý hovorí o tom že skončila inicializácia servera
    initialization_complete(_pid_info);

    // Autentifikácia 4 klientov - Zistenie kto sa napojil a zatriedenie na príslušné
    // soket deskriptory, keďže pripojenie na server môže bzť v náhodnom poradí.
    client_authentification(debug_msg, &sockets, serv_sock_decs);

    _serv_info->_sockets = sockets;

    client_authentification(debug_msg, &sockets, serv_sock_decs);

    _serv_info->_sockets = sockets;

    client_authentification(debug_msg, &sockets, serv_sock_decs);

    _serv_info->_sockets = sockets;

    client_authentification(debug_msg, &sockets, serv_sock_decs);

    _serv_info->_sockets = sockets;

    // Hlavná slučka serveru
    int sock_status;
    SOCKET_DATA in_data, out_data;
    fd_set read_fd_set;
    int status;
    while(1)
    {   
        nanosleep((const struct timespec[]){{0, 100 * NANO_SECOND_MULTIPLIER}}, NULL);
        FD_ZERO(&read_fd_set);
        FD_SET(sockets.client1_sock,&read_fd_set);
        FD_SET(sockets.client2_sock,&read_fd_set);
        FD_SET(sockets.client3_sock,&read_fd_set);
        FD_SET(sockets.client4_sock,&read_fd_set);
        sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (i)   Scanning sockets for communication.");
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        select(FD_SETSIZE,&read_fd_set,NULL,NULL,NULL);// Skenuj sokety a zisti či je niekto pripravený na posielanie informácií

        // Ak je CLIENT1 pripraveny komunikuj
        if(FD_ISSET(sockets.client1_sock, &read_fd_set))
        {
            sock_status = read(sockets.client1_sock, &in_data, sizeof(in_data));
            if (sock_status == -1)
            {
                sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Cannot read from Client 1.");
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                kill(getpid(),SIGTERMINATE);
            }
            if (sock_status == 0)
            {
                sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Client 1 disconnected");
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                kill(getpid(),SIGTERMINATE);
            }

            out_data=in_data;
            
            switch(out_data.destination)
            {
                case P_CLI1: write(sockets.client1_sock, &out_data, sizeof(out_data)); break;
                case P_CLI2: write(sockets.client2_sock, &out_data, sizeof(out_data)); break;
                case P_CLI3: write(sockets.client3_sock, &out_data, sizeof(out_data)); break;
                case P_CLI4: write(sockets.client4_sock, &out_data, sizeof(out_data)); break;
                default: out_data.control_word = SOCK_FAIL; write(sockets.client1_sock, &out_data, sizeof(out_data)); break;
            }
        }

        // Ak je CLIENT2 pripraveny komunikuj
        if(FD_ISSET(sockets.client2_sock, &read_fd_set))
        {
            sock_status = read(sockets.client2_sock, &in_data, sizeof(in_data));
            if (sock_status == -1)
            {
                sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Cannot read from Client 2.");
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                kill(getpid(),SIGTERMINATE);
            }
            if (sock_status == 0)
            {
                sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Client2 disconnected");
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                kill(getpid(),SIGTERMINATE);
            }

            out_data=in_data;
            
            switch(out_data.destination)
            {
                case P_CLI1: write(sockets.client1_sock, &out_data, sizeof(out_data)); break;
                case P_CLI2: write(sockets.client2_sock, &out_data, sizeof(out_data)); break;
                case P_CLI3: write(sockets.client3_sock, &out_data, sizeof(out_data)); break;
                case P_CLI4: write(sockets.client4_sock, &out_data, sizeof(out_data)); break;
                default: out_data.control_word = SOCK_FAIL; write(sockets.client1_sock, &out_data, sizeof(out_data)); break;
            }
        }

        // Ak je CLIENT3 pripraveny komunikuj
        if(FD_ISSET(sockets.client3_sock, &read_fd_set))
        {
            sock_status = read(sockets.client3_sock, &in_data, sizeof(in_data));
            if (sock_status == -1)
            {
                sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Cannot read from Client 2.");
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                kill(getpid(),SIGTERMINATE);
            }
            if (sock_status == 0)
            {
                sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Client3 disconnected");
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                kill(getpid(),SIGTERMINATE);
            }

            out_data=in_data;
            
            switch(out_data.destination)
            {
                case P_CLI1: write(sockets.client1_sock, &out_data, sizeof(out_data)); break;
                case P_CLI2: write(sockets.client2_sock, &out_data, sizeof(out_data)); break;
                case P_CLI3: write(sockets.client3_sock, &out_data, sizeof(out_data)); break;
                case P_CLI4: write(sockets.client4_sock, &out_data, sizeof(out_data)); break;
                default: out_data.control_word = SOCK_FAIL; write(sockets.client1_sock, &out_data, sizeof(out_data)); break;
            }
        }

        // Ak je CLIENT4 pripraveny komunikuj
        if(FD_ISSET(sockets.client4_sock, &read_fd_set))
        {
            sock_status = read(sockets.client4_sock, &in_data, sizeof(in_data));
            if (sock_status == -1)
            {
                sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Cannot read from Client 2.");
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                kill(getpid(),SIGTERMINATE);
            }
            if (sock_status == 0)
            {
                sprintf(debug_msg.mesg_text, "{SERV} [SOCK]  (!)   Client4 disconnected");
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                kill(getpid(),SIGTERMINATE);
            }

            out_data=in_data;
            
            switch(out_data.destination)
            {
                case P_CLI1: write(sockets.client1_sock, &out_data, sizeof(out_data)); break;
                case P_CLI2: write(sockets.client2_sock, &out_data, sizeof(out_data)); break;
                case P_CLI3: write(sockets.client3_sock, &out_data, sizeof(out_data)); break;
                case P_CLI4: write(sockets.client4_sock, &out_data, sizeof(out_data)); break;
                default: out_data.control_word = SOCK_FAIL; write(sockets.client1_sock, &out_data, sizeof(out_data)); break;
            }
        }        
    }
}

// Ukončovanie programu
void *thr_terminateCheck(void *args)
{
    SERV_OBJ *actual_args = args;
    PID_INFO *pid_info = &actual_args->_pid_info;
    int status;

    while(1)
    {
        DEBUG_MESSAGE debug_msg;
        debug_msg.mesg_type = 1;

        sigset_t pending;
        sigpending(&pending);
        // Ak signál SIGTERMINATE je nastavený začni ukončovanie programu
        if (sigismember(&pending, SIGTERMINATE))
        {
            // Ukonči CHILD 1
            kill(pid_info->pid1,SIGTERMINATE);
            waitpid(pid_info->pid1, NULL, 0);
            printf("{SERV}  [EXIT]  (‼)   TERMINATION CALLED.\n");
            printf("{SERV}  [EXIT]  (‼)   SENDING SIGTERMINATE TO CHILD 1 PID:%d.\n",pid_info->pid1);
            if(actual_args->_sockets.client1_sock < 0)
                printf("{SERV}  [EXIT]  (‼)   SERVER-SIDE SOCKET 1 DOES NOT EXIST.\n");
            else
                printf("{SERV}  [EXIT]  (‼)   CLOSING SERVER-SIDE SOCKET 1 SCK_DSC:%d with status '%d'.\n",actual_args->_sockets.client1_sock ,close(actual_args->_sockets.client1_sock));         
            
            // Ukonči CHILD 2
            printf("{SERV}  [EXIT]  (‼)   SENDING SIGTERMINATE TO CHILD 2 PID:%d.\n",pid_info->pid2);
            fflush(stdout);
            kill(pid_info->pid2,SIGTERMINATE);
            waitpid(pid_info->pid2, NULL, 0);
            if(actual_args->_sockets.client2_sock < 0)
                printf("{SERV}  [EXIT]  (‼)   SERVER-SIDE SOCKET 2 DOES NOT EXIST.\n");
            else
                printf("{SERV}  [EXIT]  (‼)   CLOSING SERVER-SIDE SOCKET 2 SCK_DSC:%d with status '%d'.\n",actual_args->_sockets.client2_sock ,close(actual_args->_sockets.client2_sock));        

            // Ukonči CHILD 3
            printf("{SERV}  [EXIT]  (‼)   SENDING SIGTERMINATE TO CHILD 3 PID:%d.\n",pid_info->pid3);
            fflush(stdout);
            kill(pid_info->pid3,SIGTERMINATE);
            waitpid(pid_info->pid3, NULL, 0);
            if(actual_args->_sockets.client3_sock < 0)
                printf("{SERV}  [EXIT]  (‼)   SERVER-SIDE SOCKET 3 DOES NOT EXIST.\n");
            else
                printf("{SERV}  [EXIT]  (‼)   CLOSING SERVER-SIDE SOCKET 3 SCK_DSC:%d with status '%d'.\n",actual_args->_sockets.client3_sock ,close(actual_args->_sockets.client3_sock));

            // Ukonči CHILD 4
            printf("{SERV}  [EXIT]  (‼)   SENDING SIGTERMINATE TO CHILD 4 PID:%d.\n",pid_info->pid4);
            fflush(stdout);
            kill(pid_info->pid4,SIGTERMINATE);
            waitpid(pid_info->pid4, NULL, 0);
            if(actual_args->_sockets.client4_sock < 0)
                printf("{SERV}  [EXIT]  (‼)   SERVER-SIDE SOCKET 4 DOES NOT EXIST.\n");
            else
                printf("{SERV}  [EXIT]  (‼)   CLOSING SERVER-SIDE SOCKET 4 SCK_DSC:%d with status '%d'.\n",actual_args->_sockets.client4_sock ,close(actual_args->_sockets.client4_sock));

            if(actual_args->server_sock < 0)
                printf("{SERV}  [EXIT]  (‼)   SERVER-SIDE LISTENING SOCKET DOES NOT EXIST.\n");
            else
                printf("{SERV}  [EXIT]  (‼)   CLOSING SERVER-SIDE LISTENING SOCKET SCK_DSC:%d with status '%d'.\n",actual_args->server_sock ,close(actual_args->server_sock));
        
            // MAIN/SERV Closing
            printf("{MAIN}  [EXIT]  (‼)   Delete/Close 'Run-Time' timer with status '%d'\n",timer_delete(actual_args->timer));
            printf("{MAIN}  [EXIT]  (‼)   Delete/Close mutex semaphore with status '%d'\n", pthread_mutex_destroy(actual_args->_terminal_sem));
            printf("{MAIN}  [EXIT]  (‼)   Deallocate mutex semaphore pointer with status '%d'\n", munmap(actual_args->_terminal_sem,sizeof(int*)));
            printf("{MAIN}  [EXIT]  (‼)   Remove/Close debug message queue with status '%d'\n",msgctl(actual_args->debug_info_ms_id, IPC_RMID, NULL));
            printf("\n---------------------------------------------------------------------------------------------\n");
            printf("{SERV}  [EXIT]  (i)   TOTAL APP RUNTIME: %d days %d hours %d minutes %d seconds\n",run_time.days,run_time.hours,run_time.minutes,run_time.seconds);
            printf("{SERV}  [EXIT]  (‼)   EXITING. %d -> %s",server_error,strerror(server_error));
            printf("\n---------------------------------------------------------------------------------------------\n");
            fflush(stdout);
            sleep(1);
            exit(server_error);
        }
    }
}

// Oblsužná funkcia časovaču
void thr_runtimeTimer(__sigval_t args)
{
    run_time.seconds++;
    if((run_time.seconds)>60)
    {
        run_time.seconds=0;
        run_time.minutes++;
    }

    if((run_time.minutes)>60)
    {
        run_time.minutes=0;
        run_time.hours++;
    }

    if((run_time.hours)>60)
    {
        run_time.hours=0;
        run_time.days++;
    }
}