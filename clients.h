//###########################################################################
//
// FILE:    	clients.h
//
// DESCRIPTION: Header pre clients.c
//
// AUTHOR:		Marek Sýkorka
//
//###########################################################################

#ifndef CLIENTS_H_
#define CLIENTS_H_

//
// Include
//

// Include pre použité technológie
// Graphics
#include <sys/ioctl.h>
// Thread
#include <pthread.h>
// Signal
#include <signal.h>
// IPC
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
// Sokety
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
// Timer
#include <time.h>
//Includes
#include "global_defs.h"

//
// Functions
//
// Klient 1
int client1_main(void);
void *thr_Graphics(void *args);
void initialize_graphics(int clr_scr);
void g_main_menu(void);
void g_menu1(void);
void g_menu2(void);
void g_table1(void);
void g_table2(void);
void g_ending(void);

// Klient 2
int client2_main(void);

// Klient 3
int client3_main(void);

// Klient 4
int client4_main(void);

// Ostatné
int waitForSig(int __signo);
typedef struct
{
	int client_socket;
	PIPE_INFO ithr_pipe_str;
	pthread_t graph_thred;
} CL1_OBJ;
void *thr_terminateCheck1(void *args);
typedef struct
{
	int client_socket;
} CL2_OBJ;
void *thr_terminateCheck2(void *args);
typedef struct
{
	int client_socket;
} CL3_OBJ;
void *thr_terminateCheck3(void *args);
typedef struct
{
	int client_socket;
	FILE *out_fd;
} CL4_OBJ;
void *thr_terminateCheck4(void *args);
int check_terminate_client();

#endif