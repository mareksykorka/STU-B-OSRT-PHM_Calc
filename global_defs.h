//###########################################################################
//
// FILE:    	global_defs.h
//
// DESCRIPTION: Súbor v ktorom sú zadefinované použité dátové typy, 
//              štruktúry, stavy a iné celoprojektové definície.
//
// AUTHOR:		Marek Sýkorka
//
//###########################################################################

#ifndef GLOBAL_DEFS_H_
#define GLOBAL_DEFS_H_

//
// Include
//

// Všeobecné Include
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Error handling
extern int server_error, client1_error, client2_error, client3_error, client4_error;

// Default defines
typedef int flag;
#define NANO_SECOND_MULTIPLIER  1000000  // 1 millisecond = 1,000,000 Nanoseconds

// General data formats
#define PRINTED 1
#define CALC_MODE1 2
#define CALC_MODE2 3
#define DATA_VALID 1
#define DATA_INVALID -1

// General struct typedefs
typedef struct
{
    int i_data;
    char c_data;
    float f_data;
    char s_data[200];
} SIMPLE_DATA;

typedef struct
{
    //Inputs
    float vzdialenost;
    float palivo;
    float spotreba;
    float jednotkova_cena_paliva;
    int ludia;
    //Outputs
    float dojazd;
    float potrebne_palivo;
    float cena_paliva;
    float cena_na_cloveka;
    char output_file_name[200];
    //Controls
    int data_status;
    int calculator_mode;
} DATA;

// Procceses
typedef struct
{
    int pid1;
    int pid2;
    int pid3;
    int pid4;
} PID_INFO;

// Message queue - IPC1 - debug info
typedef struct
{
    long mesg_type;
    char mesg_text[200];
} DEBUG_MESSAGE;
extern int debug_info_ms_id;

// Pipe defs - IPC1 - ITHR pipe
#include <unistd.h>
    typedef struct
    {
    flag new_s_data;
    flag new_g_data;
    int pipe_fd[2];
    } PIPE_INFO;

//Mutex semaphore - IPC2 - console blocking
#include <pthread.h>
extern pthread_mutex_t *terminal_sem;
#define term_lock() pthread_mutex_lock(terminal_sem)
#define term_unlock() pthread_mutex_unlock(terminal_sem)

// Signal - IPC2 - defines
#include <signal.h>
#define SIGINIT SIGUSR1
#define SIGTERMINATE SIGUSR2

//Control Socket defines
#include <sys/socket.h>
#include <arpa/inet.h>

    // Socket Port
    #define SOCK_PORT 5025

    // Socket data packet
    typedef struct
    {
        int source;
        int destination;
        int control_word;
        SIMPLE_DATA simple_data;
        DATA data;
    } SOCKET_DATA;

    // Socket destinations defines
    #define P_SERV 100
    #define P_CLI1 101
    #define P_CLI2 102
    #define P_CLI3 103
    #define P_CLI4 104

    // Socket info struct
    typedef struct
    {
        int client1_sock;
        int client2_sock;
        int client3_sock;
        int client4_sock;
    } SOCKET_INFO;

    // Socket Control words
    #define SOCK_AUTH_REQ 200
    #define SOCK_DATA_REQ 201
    #define SOCK_ACKW 202
    #define SOCK_FAIL 203

    #define FILE_CREATE 300
    #define FILE_APPEND 301
    #define FILE_EXISTS 302

    #define TYPE_INT 400
    #define TYPE_CHAR 401
    #define TYPE_FLOAT 402
    #define TYPE_STRING 403

// Timer
typedef struct
{
    int seconds;
    int minutes;
    int hours;
    int days;
} RUN_TIME;
extern RUN_TIME run_time;

//Graphics states defines
    // Menu states
    #define TERM_MAIN_MENU 1
    #define TERM_MODE1_MENU 2
    #define TERM_MODE2_MENU 3
    #define TERM_OUTPUT_MENU 4
    #define TERM_SAVE_MENU 5
    #define TERM_FILEEXISTS_MENU 6
    #define TERM_GETNAME_MENU 7
    #define TERM_WRITEFILE_MENU 8
    #define TERM_RETURN_PROMPT 100
    #define TERM_EXIT_PROMPT 101
    #define TERM_INVALID_PROMPT 102

    // Question states
    #define TERM_DISTANCE_Q 10
    #define TERM_FUEL_Q 11
    #define TERM_SPOTREBA_Q 12
    #define TERM_COST_Q 13
    #define TERM_PEPOLE_Q 14
    #define TERM_INVALID_Q 20

    //Term positions
    #define TERM_POS_MENU_S 1
    #define TERM_POS_MENU_E 12
    #define TERM_POS_QUES_S 12
    #define TERM_POS_QUES_E 14
    #define TERM_POS_INPT_S 14
    #define TERM_POS_INPT 15
    #define TERM_POS_INPT_E 16
    #define TERM_POS_DEBG_S 16
    #define TERM_POS_DEBG_E 48

    //Table positions
    #define TERM_TABLE_1_S 16
    #define TERM_TABLE_1_E 29
    #define TERM_TABLE_2_S 29
    #define TERM_TABLE_2_E 44
    #define TERM_TABLE_03_S 44
    #define TERM_TABLE_03_E 62
    #define TERM_TABLE_04_S 62
    #define TERM_TABLE_04_E 74
    #define TERM_TABLE_13_S 44
    #define TERM_TABLE_13_E 74
    #define TERM_TABLE_NA "N/A"

// Graphics special functions defines
#define cls() printf("\033[H\033[J")
#define clrline() printf("\x1b[2K")
#define set_cursor(x,y) printf("\033[%d;%dH", (y), (x))
#define increment(x,max) (((x)<(max-1))?((x)++):((x)=(TERM_POS_DEBG_S+2)))

// Threads
typedef struct
{
	PID_INFO _pid_info;
	pthread_mutex_t *_terminal_sem;
	int debug_info_ms_id;

	int server_sock;
	SOCKET_INFO _sockets;

    timer_t timer;
} SERV_OBJ;

#endif