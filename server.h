//###########################################################################
//
// FILE:    	server.h
//
// DESCRIPTION: Header súbor pre server.c
//
// AUTHOR:		Marek Sýkorka
//
//###########################################################################

#ifndef SERVER_H_
#define SERVER_H_

//
// Include
//

// Include pre použité technológie
#include <sys/mman.h>
// Thread
#include <pthread.h>
// Signal
#include <signal.h>
// IPC
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
// Wait
#include <sys/wait.h>
// Sokety
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
//Includes
#include "global_defs.h"

//
// Functions
//

// Inicializačné
void initialize_child_processes(PID_INFO *_pid_info);
void initialization_complete(PID_INFO *_pid_info);
// Sokety a komunikácia
int create_server_socket(DEBUG_MESSAGE debug_msg);
int client_authentification(DEBUG_MESSAGE debug_msg, SOCKET_INFO *_sockets, int _server_sock);
// Hlavná obslužná funkcia serveru
int server_main(SERV_OBJ *_serv_info);
// Ukončovanie programu
void *thr_terminateCheck(void *args);
// Oblsužná funkcia časovaču
void thr_runtimeTimer(__sigval_t args);

#endif