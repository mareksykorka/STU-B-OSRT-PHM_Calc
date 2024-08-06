//###########################################################################
//
// FILE:    	main.c
//
// DESCRIPTION: Hlavná funkcia / vstupný bod programu, obsahuje inicializáciu
//              globálnych komunikačných prostriedkov (fronta správ, mutex 
//              semafór, signály ...), rozdeľuje program do 5 procesov -
//              1 server a 4 klienti atď.
//
// AUTHOR:		Marek Sýkorka
//
//###########################################################################

//
// Include
//

// Všeobecné Include
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Include pre použité technológie
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

// Include špecifický pre program
#include "global_defs.h"
#include "server.h"

// Globálne premenné
RUN_TIME run_time;
PID_INFO pid_info;
pthread_mutex_t *terminal_sem;
int debug_info_ms_id;
int server_error = 0, client1_error = 0, client2_error = 0, client3_error = 0, client4_error = 0;

//
// Functions
//
int main()
{
    //printf("(‼)");
    // Lokálne premenné
    SERV_OBJ serv_closable_obj;
    DEBUG_MESSAGE debug_msg;
    debug_msg.mesg_type = 1;

    // Inicializácia štruktúry run_time - zaznamenáva čas vykonávania programu.
    run_time.seconds = 0;
    run_time.minutes = 0;
    run_time.hours = 0;
    run_time.days = 0;

    // Nastavenie činnosti po uplynutí času časovača - zavolaj vlákno thr_runtimeTimer
    struct sigevent timer_event;
    timer_event.sigev_notify=SIGEV_THREAD;
    timer_event._sigev_un._sigev_thread._function = thr_runtimeTimer;
    
    // Vytvorenie REALTIME časovača
    timer_t timer;
    if(timer_create(CLOCK_REALTIME, &timer_event, &timer) != 0)
    {
        printf("{MAIN} [TIME]  (!)   'timer_create' failed to create timer.\n");
        printf("{MAIN} [EXIT]  (‼)   EXITING. Error code: %d -> %s\n",errno,strerror(errno));
        fflush(stdout);
        exit errno;
    }
    printf("{MAIN} [TIME]  (i)   'timer_create' created timer.\n");
    fflush(stdout);

    // Nastavenie času pre časovač a jeho spustenie
    struct itimerspec time;
    time.it_value.tv_sec=1; // Prvý čas časovača = 1 sec
    time.it_value.tv_nsec=0;
    time.it_interval.tv_sec=1; // Potom opakuj ďalej po 1 sec
    time.it_interval.tv_nsec=0;
    if(timer_settime(timer,CLOCK_REALTIME,&time,NULL) != 0)
    {
        printf("{MAIN} [TIME]  (!)   'timer_create' failed to create timer.\n");
        printf("{MAIN} [TIME]  (!)   Delete timer with status '%d'\n",timer_delete(timer));
        printf("{MAIN} [EXIT]  (‼)   EXITING. Error code: %d -> %s\n",errno,strerror(errno));
        fflush(stdout);
        exit errno;
    }
    serv_closable_obj.timer=timer;
    printf("{MAIN} [TIME]  (i)   'timer_create' created timer.\n");
    fflush(stdout);

    // Globálne ignorovanie signálu SIGPIPE, ktorý vzniká pri pokazení IPC1 Pipe, 
    // alebo pri chybnej komunikácii cez sokety, tieto chyby sú obslúžené inak.
    signal(SIGPIPE, SIG_IGN);

    // Globálna príprava signálu SIGTERMINATE, ktorý sa používa na ukončenie aplikácie
    // Príprava spočíva v prispôsobení signálu na to aby naň bolo možné čakať pomocou sigwait();
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGTERMINATE);
    pthread_sigmask(SIG_BLOCK, &blocked, NULL);

    // Vytvorenie medziprocesorového mutex semafóru - blokovanie prístupu ku terminálu
    // Vytvorenie pointera na semafór v časti pamäte prístupnej medzi procesmi.
    terminal_sem = (pthread_mutex_t*) mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if(terminal_sem == MAP_FAILED)
    {
        printf("{MAIN} [TIME]  (!)   'mmap' failed to create shared memmory mapped pointer for mutex semaphore.\n");
        printf("{MAIN} [TIME]  (!)   Delete timer with status '%d'\n",timer_delete(timer));
        printf("{MAIN} [EXIT]  (‼)   EXITING. Error code: %d -> %s\n",errno,strerror(errno));
        fflush(stdout);
        exit errno;
    }
    printf("{MAIN} [TIME]  (i)   'mmap' created shared memmory mapped pointer for mutex semaphore.\n");
    fflush(stdout);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED); // Semafór nech je shared
    if (pthread_mutex_init(terminal_sem, &attr) != 0)
    {
        printf("{MAIN} [IPC2]  (!)   'pthread_mutex_init' failed to initialize mutex semaphore.\n");
        printf("{MAIN} [TIME]  (!)   Delete timer with status '%d'\n",timer_delete(timer));
        printf("{MAIN} [TIME]  (!)   Deallocate mutex semaphore pointer with status '%d'\n", munmap(terminal_sem,sizeof(int*)));
        printf("{MAIN} [EXIT]  (‼)   EXITING. Error code: %d -> %s\n",errno,strerror(errno));
        fflush(stdout);
        pthread_mutexattr_destroy(&attr);
        exit errno;
    }
    printf("{MAIN} [IPC2]  (i)   'pthread_mutex_init' initialized mutex semaphore.\n");
    fflush(stdout);

    pthread_mutexattr_destroy(&attr);
    serv_closable_obj._terminal_sem = terminal_sem;

    // Vytvorenie fronty správ ktorá sa používa na zasielanie debug informácií zo všetkých procesov grafickému vláknu
    if ((debug_info_ms_id = msgget(IPC_PRIVATE, 0600)) == -1) // Preco IPC_PRIVATE a 0600 -> https://man7.org/linux/man-pages/man2/msgget.2.html
    {
        printf("{MAIN} [IPC1]  (!)   'msgget' failed to create message queue.\n");
        printf("{MAIN} [TIME]  (!)   Delete timer with status '%d'\n",timer_delete(timer));
        printf("{MAIN} [TIME]  (!)   Destroying mutex semaphore with status '%d'\n", pthread_mutex_destroy(terminal_sem));
        printf("{MAIN} [TIME]  (!)   Deallocate mutex semaphore pointer with status '%d'\n", munmap(terminal_sem,sizeof(int*)));
        printf("{MAIN} [EXIT]  (‼)   EXITING. Error code: %d -> %s\n",errno,strerror(errno));
        pthread_mutex_destroy(serv_closable_obj._terminal_sem);
        fflush(stdout);
        exit errno;
    }
    serv_closable_obj.debug_info_ms_id = debug_info_ms_id;
    sprintf(debug_msg.mesg_text, "{MAIN} [IPC1]  (i)   'msgget' CREATED 'debug' message queue.");
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Vytvorenie "terminačného vlákna" v ktorom sa čaká na signál SIGTERMINATE
    sprintf(debug_msg.mesg_text, "{SERV} [THRD]  (i)   Creating termination thread.");
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    pthread_t tid1;
    if (pthread_create(&tid1,NULL,thr_terminateCheck,&serv_closable_obj) != 0)
    {
        printf("{MAIN} [IPC1]  (!)   'pthread_create' failed to create thread.\n");
        printf("{MAIN} [TIME]  (!)   Delete timer with status '%d'\n",timer_delete(timer));
        printf("{MAIN} [TIME]  (!)   Destroying mutex semaphore with status '%d'\n", pthread_mutex_destroy(terminal_sem));
        printf("{MAIN} [TIME]  (!)   Deallocate mutex semaphore pointer with status '%d'\n", munmap(terminal_sem,sizeof(int*)));
        printf("{MAIN} [EXIT]  (!)   Remove/Close debug message queue with status '%d'\n",msgctl(debug_info_ms_id, IPC_RMID, NULL));
        printf("{MAIN} [EXIT]  (‼)   EXITING. Error code: %d -> %s\n",errno,strerror(errno));
        fflush(stdout);
        exit errno;
    }
    sprintf(debug_msg.mesg_text, "{SERV} [THRD]  (i)   Termination thread CREATED succesfully.");
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Vytvorenie child procesov
    initialize_child_processes(&pid_info);
    serv_closable_obj._pid_info = pid_info;

    // Server obsluha
    if(pid_info.pid1 != 0 && pid_info.pid2 != 0 && pid_info.pid3 != 0 && pid_info.pid4 != 0)
    {
        server_main(&serv_closable_obj);
    }
}