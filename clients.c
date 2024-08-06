//###########################################################################
//
// FILE:    	clients.c
//
// DESCRIPTION: Funkcie ktoré používajú CHLD procesy, klienti.
//
// AUTHOR:		Marek Sýkorka
//
//###########################################################################

//
// Include
//

// Include špecifický pre program
#include "global_defs.h"
#include "clients.h"

//
// Functions
//

// Klient 1
int client1_main(void)
{
    // Lokálne premenné
    SOCKET_DATA in_data, out_data, pipe_data;
    CL1_OBJ cl1_closable_obj;
    PIPE_INFO ithr_pipe_str;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;

    sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (i)   1:%d alive.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Vytvorenie "terminačného vlákna" v ktorom sa čaká na signál SIGTERMINATE
    sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (i)   1:%d Creating termination thread.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    pthread_t dstr_thrd;
    if (pthread_create(&dstr_thrd,NULL,thr_terminateCheck1,&cl1_closable_obj) != 0)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (!)   1:%d failed to create termination thread.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client1_error = errno;
        kill(getppid(),SIGTERMINATE);
    }
    sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (i)   1:%d Termination thread CREATED succesfully.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Vytvorenie medzivláknovej rúry
    ithr_pipe_str.new_g_data = 0;
    ithr_pipe_str.new_s_data = 0;
    if(pipe(ithr_pipe_str.pipe_fd) == -1)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [IPC1]  (!)   1:%d ITHR pipe can not be created.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client1_error = errno;
        kill(getppid(),SIGTERMINATE);
    }
    cl1_closable_obj.ithr_pipe_str = ithr_pipe_str;

    sprintf(debug_msg.mesg_text, "{CHLD} [IPC1]  (i)   1:%d ITHR pipe CREATED succesfully.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    //Create Graphic Thread
    sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (i)   1:%d Creating graphic thread.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    pthread_t grphc_thrd;
    if (pthread_create(&grphc_thrd,NULL,thr_Graphics,&ithr_pipe_str) != 0)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (!)   1:%d failed to create graphic thread.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client1_error = errno;
        kill(getppid(),SIGTERMINATE);
    }
    sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (i)   1:%d Graphic thread CREATED succesfully.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    cl1_closable_obj.graph_thred = grphc_thrd;

    // Wait for Initialization signal to arrive from server
    if(waitForSig(SIGINIT))
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [PROC]  (i)   1:%d Init complete. Initialization signal arrived. Child starting.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    }

    // Create client socket
    int d_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    cl1_closable_obj.client_socket = d_client_socket;
    // Check creation status
    if (d_client_socket == -1)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   1:%d Cannot create client socket.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client1_error = errno;
        kill(getppid(),SIGTERMINATE);
    }
    // Socket creation succesful
    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   1:%d Client socket %d created.",getpid(),d_client_socket);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Setting socket
    struct sockaddr_in client;  
    memset(&client, 0, sizeof(client));  
    client.sin_family = AF_INET;  
    client.sin_addr.s_addr = inet_addr("127.0.0.1");  
    client.sin_port = htons(SOCK_PORT);

     // Connecting to client with created socket
    if (connect(d_client_socket, (struct sockaddr*)&client, sizeof(client)) != 0)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   1:%d Cannot connect to 127.0.0.1:%d.",getpid(),SOCK_PORT);
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client1_error = errno;
        kill(getppid(),SIGTERMINATE);
    }

    // Socket connection succesful
    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   1:%d Connected to 127.0.0.1:%d.",getpid(),SOCK_PORT);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   1:%d Waiting for AUTH request.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    while(in_data.control_word != SOCK_AUTH_REQ)
    {
        if (read(d_client_socket, &in_data, sizeof(in_data)) < 0)
        {
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   1:%d Socket FAILED.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            client1_error = errno;
            kill(getppid(),SIGTERMINATE);     
        }
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (>)   1:%d AUTH request received.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

        if(in_data.control_word == SOCK_AUTH_REQ)
        {
            out_data.control_word = P_CLI1;
            if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   1:%d Socket FAILED.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client1_error = errno;
                kill(getppid(),SIGTERMINATE);       
            }
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (<)   1:%d AUTH message sent.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            
            if (read(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   1:%d Socket FAILED.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client1_error = errno;
                kill(getppid(),SIGTERMINATE);
            }

            if(out_data.control_word == SOCK_ACKW)
            {     
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   1:%d AUTH succesful ACKW received.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            }
            else
            {     
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   1:%d AUTH not succesful.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client1_error = EINVAL;
                kill(getppid(),SIGTERMINATE);
            }
        }
    }

    while(1)
    {
        // Every loop clear all data structs
        memset(&in_data, 0, sizeof(in_data));
        memset(&out_data, 0, sizeof(out_data));
        memset(&pipe_data, 0, sizeof(pipe_data));

        if((ithr_pipe_str.new_g_data == 1) && (ithr_pipe_str.new_s_data == 0))
        {
            // Read ITHR pipe
            if (read(ithr_pipe_str.pipe_fd[0], &pipe_data, sizeof(pipe_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [IPC1]  (!)   %d Pipe FAILED.\n",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                // ERROR pipe can not communicate - terminate app
                kill(getppid(),SIGTERMINATE);          
            }
            
            out_data = pipe_data;
            if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   %d Socket FAILED.\n",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                // ERROR socket can not communicate - terminate app
                kill(getppid(),SIGTERMINATE);
            }

            // Read Socket
            if (read(d_client_socket, &in_data, sizeof(in_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   %d Socket FAILED.\n",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                // ERROR socket can not communicate - terminate app
                kill(getppid(),SIGTERMINATE);          
            }

            pipe_data = in_data;
            if (write(ithr_pipe_str.pipe_fd[1], &pipe_data, sizeof(pipe_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [IPC1]  (!)   %d Pipe FAILED.\n",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                // ERROR pipe can not communicate - terminate app
                kill(getppid(),SIGTERMINATE);
            }

            ithr_pipe_str.new_s_data = 1;
        }
    }
}

void *thr_Graphics(void *args)
{
    // Local Variable Defines
    SOCKET_DATA full_data, pipe_data;
    PIPE_INFO *pipe_info = args;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;

    #ifdef DEBUG_INFO 
    FILE *fp;
    fp = fopen("debug.log", "w");
    #endif

    int menu_stat = TERM_MAIN_MENU, old_menu_stat;
    int data_stat = TERM_POS_MENU_S+2;
    int print_stat = 1; int out_stat = PRINTED;
    int debug_line = TERM_POS_DEBG_S+2, msg_num=1;
    
    struct winsize w_old, w_new;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w_old);

    initialize_graphics(1);

    while(1)
    {
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w_new);

        struct msqid_ds msg_info;
        msgctl(debug_info_ms_id, IPC_STAT, &msg_info);
        for(int i = 0; i < msg_info.msg_qnum; i++)
        {
            nanosleep((const struct timespec[]){{0, 25 * NANO_SECOND_MULTIPLIER}}, NULL);
            if(msgrcv(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 1, 0) > 0)
            {
                term_lock();
                set_cursor(0,debug_line);
                clrline();
                printf("%3d  %s",msg_num++, debug_msg.mesg_text);
                #ifdef DEBUG_INFO 
                fprintf(fp, "%3d  %s\n",msg_num++, debug_msg.mesg_text);
                #endif            
                fflush(stdout);
                increment(debug_line, w_new.ws_row);
                set_cursor(3,TERM_POS_INPT);
                term_unlock();
            }
        }

        switch(menu_stat)
        {
            case TERM_MAIN_MENU:
            {
                g_main_menu();

                if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                {
                    initialize_graphics(0);

                    memset(&full_data, 0, sizeof(full_data));
                    memset(&pipe_data, 0, sizeof(pipe_data));

                    pipe_data.source = P_CLI1;
                    pipe_data.destination = P_CLI2;
                    pipe_data.control_word = SOCK_DATA_REQ;
                    write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                    pipe_info->new_g_data = 1;
                }                
                if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                {
                    read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                    
                    switch (pipe_data.simple_data.c_data)
                    {
                        case 'D': 
                        case 'd': menu_stat = TERM_MODE1_MENU; break;
                        case 'P':
                        case 'p': menu_stat = TERM_MODE2_MENU; break;
                        case 'E':
                        case 'e': menu_stat = TERM_EXIT_PROMPT; break;
                        default: menu_stat = TERM_INVALID_PROMPT; break;
                    }

                    pipe_info->new_g_data = 0;
                    pipe_info->new_s_data = 0;
                }
                break;
            }
            case TERM_MODE1_MENU:
            {
                if(print_stat == 1)
                {
                    initialize_graphics(0);
                    g_menu1();
                    print_stat = 0;
                }

                if(data_stat == 3 && menu_stat == TERM_MODE1_MENU)
                {
                    //Prvy param
                    term_lock();
                    set_cursor(0,data_stat);
                    printf("->");
                    set_cursor(0,TERM_POS_QUES_S+1);
                    clrline();
                    printf("Zadajte desatinné číslo.");
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock();

                    if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                    {
                        pipe_data.source = P_CLI1;
                        pipe_data.destination = P_CLI2;
                        pipe_data.control_word = SOCK_DATA_REQ;
                        write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                        pipe_info->new_g_data = 1;
                    }
                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                        
                        switch (pipe_data.simple_data.c_data)
                        {
                            case 'R':
                            case 'r': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_RETURN_PROMPT; break;
                            case 'E':
                            case 'e': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_EXIT_PROMPT; break;
                        }

                        full_data.data.palivo = pipe_data.simple_data.f_data;

                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                        data_stat++;
                    }
                }

                if(data_stat == 4 && menu_stat == TERM_MODE1_MENU)
                {
                    //Druhy param
                    term_lock();
                    set_cursor(0,data_stat-1);
                    printf("  ");
                    set_cursor(0,data_stat);
                    printf("->");
                    set_cursor(0,TERM_POS_QUES_S+1);
                    clrline();
                    printf("Zadajte desatinné číslo.");
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock();

                    if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                    {
                        pipe_data.source = P_CLI1;
                        pipe_data.destination = P_CLI2;
                        pipe_data.control_word = SOCK_DATA_REQ;
                        write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                        pipe_info->new_g_data = 1;
                    }
                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                        
                        switch (pipe_data.simple_data.c_data)
                        {
                            case 'R':
                            case 'r': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_RETURN_PROMPT; break;
                            case 'E':
                            case 'e': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_EXIT_PROMPT; break;
                        }

                        full_data.data.spotreba = pipe_data.simple_data.f_data;

                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                        data_stat++;
                    }
                }
                
                if(data_stat == 5 && menu_stat == TERM_MODE1_MENU)
                {
                    //Treti param
                    term_lock();
                    set_cursor(0,data_stat-1);
                    printf("  ");
                    set_cursor(0,data_stat);
                    printf("->");
                    set_cursor(0,TERM_POS_QUES_S+1);
                    clrline();
                    printf("Zadajte desatinné číslo. *(Pre ignorovanie stlačte ENTER.)");
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock();

                    if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                    {
                        pipe_data.source = P_CLI1;
                        pipe_data.destination = P_CLI2;
                        pipe_data.control_word = SOCK_DATA_REQ;
                        write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                        pipe_info->new_g_data = 1;
                    }
                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                        
                        switch (pipe_data.simple_data.c_data)
                        {
                            case 'R':
                            case 'r': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_RETURN_PROMPT; break;
                            case 'E':
                            case 'e': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_EXIT_PROMPT; break;
                        }

                        full_data.data.jednotkova_cena_paliva = pipe_data.simple_data.f_data;

                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                        data_stat++;
                    }
                }
                
                if(data_stat == 6 && menu_stat == TERM_MODE1_MENU)
                {
                    //Stvrty param
                    term_lock();
                    set_cursor(0,data_stat-1);
                    printf("  ");
                    set_cursor(0,data_stat);
                    printf("->");
                    set_cursor(0,TERM_POS_QUES_S+1);
                    clrline();
                    printf("Zadajte celé číslo. *(Pre ignorovanie stlačte ENTER.)");
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock(); 

                    if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                    {
                        pipe_data.source = P_CLI1;
                        pipe_data.destination = P_CLI2;
                        pipe_data.control_word = SOCK_DATA_REQ;
                        write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                        pipe_info->new_g_data = 1;
                    }
                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                        
                        switch (pipe_data.simple_data.c_data)
                        {
                            case 'R':
                            case 'r': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_RETURN_PROMPT; break;
                            case 'E':
                            case 'e': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_EXIT_PROMPT; break;
                        }

                        if(menu_stat != TERM_MODE1_MENU)
                            break;

                        full_data.data.ludia = pipe_data.simple_data.i_data;

                        sprintf(debug_msg.mesg_text, "{CHLD} [INPT]  (i)   GRAPH Client %f l/km %f l %f eur %d #.",pipe_data.data.spotreba,pipe_data.data.palivo,pipe_data.data.jednotkova_cena_paliva,pipe_data.data.ludia);
                        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

                        term_lock();
                        set_cursor(0,data_stat);
                        data_stat = TERM_POS_MENU_S+2;
                        printf("  ");
                        fflush(stdout);
                        set_cursor(3,TERM_POS_INPT);
                        term_unlock();

                        full_data.data.calculator_mode = CALC_MODE1;
                        full_data.data.data_status = DATA_VALID;
                        full_data.source = P_CLI1;
                        full_data.destination = P_CLI3;
                        full_data.control_word = SOCK_DATA_REQ;
                        menu_stat = TERM_OUTPUT_MENU;
                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                        print_stat = 1;
                        write(pipe_info->pipe_fd[1], &full_data, (sizeof(full_data)));
                        pipe_info->new_g_data = 1;
                    }
                }

                sprintf(debug_msg.mesg_text, "{CHLD} [INPT]  (i)   GRAPH client char:%c int:%i float:%f\n",full_data.simple_data.c_data,full_data.simple_data.i_data,full_data.simple_data.f_data);
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

                break;
            }
            case TERM_MODE2_MENU:
            {
                if(print_stat == 1)
                {
                    initialize_graphics(0);
                    g_menu2();
                    print_stat = 0;
                }
                
                if(data_stat == 3 && menu_stat == TERM_MODE2_MENU)
                {
                    //Prvy param
                    term_lock();
                    set_cursor(0,data_stat);
                    printf("->");
                    set_cursor(0,TERM_POS_QUES_S+1);
                    clrline();
                    printf("Zadajte desatinné číslo.");
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock();

                    if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                    {
                        pipe_data.source = P_CLI1;
                        pipe_data.destination = P_CLI2;
                        pipe_data.control_word = SOCK_DATA_REQ;
                        write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                        pipe_info->new_g_data = 1;
                    }
                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                        
                        switch (pipe_data.simple_data.c_data)
                        {
                            case 'R':
                            case 'r': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_RETURN_PROMPT; break;
                            case 'E':
                            case 'e': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_EXIT_PROMPT; break;
                        }

                        full_data.data.vzdialenost = pipe_data.simple_data.f_data;

                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                        data_stat++;
                    }
                }

                if(data_stat == 4 && menu_stat == TERM_MODE2_MENU)
                {
                    //Druhy param
                    term_lock();
                    set_cursor(0,data_stat-1);
                    printf("  ");
                    set_cursor(0,data_stat);
                    printf("->");
                    set_cursor(0,TERM_POS_QUES_S+1);
                    clrline();
                    printf("Zadajte desatinné číslo.");
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock();

                    if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                    {
                        pipe_data.source = P_CLI1;
                        pipe_data.destination = P_CLI2;
                        pipe_data.control_word = SOCK_DATA_REQ;
                        write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                        pipe_info->new_g_data = 1;
                    }
                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                        
                        switch (pipe_data.simple_data.c_data)
                        {
                            case 'R':
                            case 'r': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_RETURN_PROMPT; break;
                            case 'E':
                            case 'e': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_EXIT_PROMPT; break;
                        }

                        full_data.data.spotreba = pipe_data.simple_data.f_data;

                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                        data_stat++;
                    }
                }
                
                if(data_stat == 5 && menu_stat == TERM_MODE2_MENU)
                {
                    //Treti param
                    term_lock();
                    set_cursor(0,data_stat-1);
                    printf("  ");
                    set_cursor(0,data_stat);
                    printf("->");
                    set_cursor(0,TERM_POS_QUES_S+1);
                    clrline();
                    printf("Zadajte desatinné číslo. *(Pre ignorovanie stlačte ENTER.)");
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock();

                    if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                    {
                        pipe_data.source = P_CLI1;
                        pipe_data.destination = P_CLI2;
                        pipe_data.control_word = SOCK_DATA_REQ;
                        write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                        pipe_info->new_g_data = 1;
                    }
                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                        
                        switch (pipe_data.simple_data.c_data)
                        {
                            case 'R':
                            case 'r': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_RETURN_PROMPT; break;
                            case 'E':
                            case 'e': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_EXIT_PROMPT; break;
                        }

                        full_data.data.jednotkova_cena_paliva = pipe_data.simple_data.f_data;

                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                        data_stat++;
                    }
                }
                
                if(data_stat == 6 && menu_stat == TERM_MODE2_MENU)
                {
                    //Stvrty param
                    term_lock();
                    set_cursor(0,data_stat-1);
                    printf("  ");
                    set_cursor(0,data_stat);
                    printf("->");
                    set_cursor(0,TERM_POS_QUES_S+1);
                    clrline();
                    printf("Zadajte celeéčíslo. *(Pre ignorovanie stlačte ENTER.)");
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock(); 

                    if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                    {
                        pipe_data.source = P_CLI1;
                        pipe_data.destination = P_CLI2;
                        pipe_data.control_word = SOCK_DATA_REQ;
                        write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                        pipe_info->new_g_data = 1;
                    }
                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                        
                        switch (pipe_data.simple_data.c_data)
                        {
                            case 'R':
                            case 'r': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_RETURN_PROMPT; break;
                            case 'E':
                            case 'e': 
                                old_menu_stat = menu_stat;
                                menu_stat = TERM_EXIT_PROMPT; break;
                        }

                        if(menu_stat != TERM_MODE2_MENU)
                            break;

                        full_data.data.ludia = pipe_data.simple_data.i_data;

                        term_lock();
                        set_cursor(0,data_stat);
                        data_stat = TERM_POS_MENU_S+2;
                        printf("  ");
                        fflush(stdout);
                        set_cursor(3,TERM_POS_INPT);
                        term_unlock();

                        full_data.data.calculator_mode = CALC_MODE2;
                        full_data.data.data_status = DATA_VALID;
                        full_data.source = P_CLI1;
                        full_data.destination = P_CLI3;
                        full_data.control_word = SOCK_DATA_REQ;
                        menu_stat = TERM_OUTPUT_MENU;
                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                        print_stat = 1;
                        write(pipe_info->pipe_fd[1], &full_data, (sizeof(full_data)));
                        pipe_info->new_g_data = 1;
                    }
                }

                sprintf(debug_msg.mesg_text, "{CHLD} [INPT]  (i)   GRAPH client char:%c int:%i float:%f\n",full_data.simple_data.c_data,full_data.simple_data.i_data,full_data.simple_data.f_data);
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

                break;
            }
            case TERM_OUTPUT_MENU:
            {
                if(out_stat == PRINTED)
                {
                    initialize_graphics(0);

                    if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                    {
                        read(pipe_info->pipe_fd[0], &full_data, sizeof(full_data));

                        out_stat = full_data.control_word;

                        pipe_info->new_g_data = 0;
                        pipe_info->new_s_data = 0;
                    }
                }

                if(out_stat == CALC_MODE1)
                {
                    g_table1();

                    term_lock();
                    //Input
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_1_S+2,TERM_POS_MENU_S+5);
                    if(full_data.data.palivo == 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.palivo);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_2_S+2,TERM_POS_MENU_S+5);
                    if(full_data.data.spotreba == 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.spotreba);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_03_S+2,TERM_POS_MENU_S+5);
                    if(full_data.data.jednotkova_cena_paliva == 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.jednotkova_cena_paliva);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_04_S+2,TERM_POS_MENU_S+5);
                    if(full_data.data.ludia == 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%d", full_data.data.ludia);
                    //Output
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_1_S+2,TERM_POS_MENU_S+9);
                    if(full_data.data.dojazd <= 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.dojazd);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_2_S+2,TERM_POS_MENU_S+9);
                    if(full_data.data.cena_paliva <= 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.cena_paliva);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_13_S+2,TERM_POS_MENU_S+9);
                    if(full_data.data.cena_na_cloveka <= 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.cena_na_cloveka);
                    //---------------------------------------------------------
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock();
                    out_stat = PRINTED;
                    menu_stat = TERM_SAVE_MENU; 
                }

                if(out_stat == CALC_MODE2)
                {
                    g_table2();

                    term_lock();
                    //Input
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_1_S+2,TERM_POS_MENU_S+5);
                    if(full_data.data.vzdialenost == 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.vzdialenost);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_2_S+2,TERM_POS_MENU_S+5);
                    if(full_data.data.spotreba == 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.spotreba);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_03_S+2,TERM_POS_MENU_S+5);
                    if(full_data.data.jednotkova_cena_paliva == 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.jednotkova_cena_paliva);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_04_S+2,TERM_POS_MENU_S+5);
                    if(full_data.data.ludia == 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%d", full_data.data.ludia);
                    //---------------------------------------------------------
                    //Output
                    set_cursor(TERM_TABLE_1_S+2,TERM_POS_MENU_S+9);
                    if(full_data.data.potrebne_palivo <= 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.potrebne_palivo);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_2_S+2,TERM_POS_MENU_S+9);
                    if(full_data.data.cena_paliva <= 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.cena_paliva);
                    //---------------------------------------------------------
                    set_cursor(TERM_TABLE_13_S+2,TERM_POS_MENU_S+9);
                    if(full_data.data.cena_na_cloveka <= 0)
                        printf(TERM_TABLE_NA);
                    else
                        printf("%.2f", full_data.data.cena_na_cloveka);
                    fflush(stdout);
                    set_cursor(3,TERM_POS_INPT);
                    term_unlock();
                    out_stat = PRINTED;
                    menu_stat = TERM_SAVE_MENU;
                }

                if(out_stat == SOCK_FAIL)
                    menu_stat = TERM_INVALID_PROMPT;

                break;
            }
            case TERM_SAVE_MENU:
            {
                set_cursor(0,TERM_POS_QUES_S+1);
                clrline();
                printf("Chcete uložiť výstup kalkulačky do súboru (Y)es/(N)o ?");
                set_cursor(3,TERM_POS_INPT);
                fflush(stdout);
                term_unlock();

                if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                {
                    initialize_graphics(0);

                    pipe_data.source = P_CLI1;
                    pipe_data.destination = P_CLI2;
                    pipe_data.control_word = SOCK_DATA_REQ;
                    write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                    pipe_info->new_g_data = 1;
                }                
                if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                {
                    read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                    
                    switch (pipe_data.simple_data.c_data)
                    {
                        case 'Y': 
                        case 'y': menu_stat = TERM_FILEEXISTS_MENU; break;
                        case 'N':
                        case 'n':
                        default: menu_stat = TERM_INVALID_PROMPT; break;
                    }

                    pipe_info->new_g_data = 0;
                    pipe_info->new_s_data = 0;
                }
                break;
            }
            case TERM_FILEEXISTS_MENU:
            {
                set_cursor(0,TERM_POS_QUES_S+1);
                clrline();
                printf("Kontrolovanie existencie výstupného súboru.");
                set_cursor(3,TERM_POS_INPT);
                fflush(stdout);
                term_unlock();

                if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                {
                    initialize_graphics(0);

                    pipe_data.source = P_CLI1;
                    pipe_data.destination = P_CLI4;
                    pipe_data.control_word = TERM_FILEEXISTS_MENU;
                    write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                    pipe_info->new_g_data = 1;
                }                
                if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                {
                    read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                    
                    switch (pipe_data.simple_data.i_data)
                    {
                        case 1:
                        {
                            set_cursor(0,TERM_POS_QUES_S+1);
                            clrline();
                            printf("Súbor existuje.");
                            set_cursor(3,TERM_POS_INPT);
                            fflush(stdout);
                            term_unlock();
                            menu_stat = TERM_WRITEFILE_MENU;
                            break;
                        }
                        case 0:
                        {
                            set_cursor(0,TERM_POS_QUES_S+1);
                            clrline();
                            printf("Súbor neexistuje.");
                            set_cursor(3,TERM_POS_INPT);
                            fflush(stdout);
                            term_unlock();
                            menu_stat = TERM_GETNAME_MENU;
                            break;
                        }  
                        default: menu_stat = TERM_INVALID_PROMPT; break;
                    }

                    pipe_info->new_g_data = 0;
                    pipe_info->new_s_data = 0;
                }
                break;
            }
            case TERM_GETNAME_MENU:
            {
                set_cursor(0,TERM_POS_QUES_S+1);
                clrline();
                printf("Zadajte názov súboru.");
                set_cursor(3,TERM_POS_INPT);
                fflush(stdout);
                term_unlock();

                if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                {
                    initialize_graphics(0);

                    pipe_data.source = P_CLI1;
                    pipe_data.destination = P_CLI2;
                    pipe_data.control_word = SOCK_DATA_REQ;
                    write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                    pipe_info->new_g_data = 1;
                }                
                if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                {
                    read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                    
                    strcpy(full_data.simple_data.s_data,pipe_data.simple_data.s_data);

                    pipe_info->new_g_data = 0;
                    pipe_info->new_s_data = 0;

                    menu_stat = TERM_WRITEFILE_MENU;
                }
                break;
            }
            case TERM_WRITEFILE_MENU:
            {
                if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                {
                    initialize_graphics(0);

                    full_data.source = P_CLI1;
                    full_data.destination = P_CLI4;
                    full_data.control_word = TERM_WRITEFILE_MENU;
                    write(pipe_info->pipe_fd[1], &full_data, (sizeof(full_data)));
                    pipe_info->new_g_data = 1;
                }                
                if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                {
                    read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));

                    pipe_info->new_g_data = 0;
                    pipe_info->new_s_data = 0;
                }
                menu_stat = TERM_INVALID_PROMPT;
                break;
            }
            case TERM_RETURN_PROMPT:
            {
                term_lock();
                set_cursor(0,TERM_POS_QUES_S+1);
                clrline();
                printf("Vrátiť sa do hlavného menu ? Aktuálne dáta sa zahodia. (Y)es/(N)o");
                set_cursor(3,TERM_POS_INPT);
                fflush(stdout);
                term_unlock();

                if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                {
                    initialize_graphics(0);

                    pipe_data.source = P_CLI1;
                    pipe_data.destination = P_CLI2;
                    pipe_data.control_word = SOCK_DATA_REQ;
                    write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                    pipe_info->new_g_data = 1;
                }                
                if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                {
                    read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                    
                    switch (pipe_data.simple_data.c_data)
                    {
                        case 'Y': 
                        case 'y': data_stat = TERM_POS_MENU_S+2; print_stat = 1; menu_stat = TERM_MAIN_MENU; break;
                        case 'N':
                        case 'n':
                        default: data_stat--; menu_stat = old_menu_stat; break;
                    }

                    pipe_info->new_g_data = 0;
                    pipe_info->new_s_data = 0;
                }
                break;
            } 
            case TERM_EXIT_PROMPT: 
            {
                term_lock();
                set_cursor(0,TERM_POS_QUES_S+1);
                clrline();
                printf("Ukončiť program ? (Y)es/(N)o");
                fflush(stdout);
                set_cursor(3,TERM_POS_INPT);
                term_unlock();

                if((pipe_info->new_g_data == 0) && (pipe_info->new_s_data == 0))
                {
                    initialize_graphics(0);

                    pipe_data.source = P_CLI1;
                    pipe_data.destination = P_CLI2;
                    pipe_data.control_word = SOCK_DATA_REQ;
                    write(pipe_info->pipe_fd[1], &pipe_data, (sizeof(pipe_data)));
                    pipe_info->new_g_data = 1;
                }                
                if((pipe_info->new_g_data == 1) && (pipe_info->new_s_data == 1))
                {
                    read(pipe_info->pipe_fd[0], &pipe_data, sizeof(pipe_data));
                    
                    switch (pipe_data.simple_data.c_data)
                    {
                        case 'Y': 
                        case 'y': g_ending(); kill(getppid(),SIGTERMINATE); break;
                        case 'N':
                        case 'n':
                        default: data_stat--; menu_stat = old_menu_stat; break;
                    }

                    pipe_info->new_g_data = 0;
                    pipe_info->new_s_data = 0;
                }
                break;
            } 
            case TERM_INVALID_PROMPT: 
            {
                initialize_graphics(0);
                menu_stat = TERM_MAIN_MENU; data_stat = TERM_POS_MENU_S+2;
                print_stat = 1; 
                memset(&full_data, 0, sizeof(full_data));
                memset(&pipe_data, 0, sizeof(pipe_data));
                break;
            }
            default: menu_stat = TERM_INVALID_PROMPT; break;
        }
    }
}

void initialize_graphics(int clr_scr)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    char menu_title[] = " PHM - Calculator ";
    int l_menu = strlen(menu_title);

    char question_title[] = " Question Box ";
    int l_question = strlen(question_title);

    char input_title[] = " Input Box ";
    int l_input = strlen(input_title);

    char debug_title[] = " Debug Info ";
    int l_debug = strlen(debug_title);

    if(clr_scr == 1)
    {
        term_lock();
        for (int i=0;i<w.ws_col;i++)
            printf("\n");
    }

    set_cursor(0,TERM_POS_MENU_S);
    for (int i=0;i<w.ws_col;i++)
        printf("#");

    set_cursor(((w.ws_col/2) - (l_menu/2)),TERM_POS_MENU_S);
    printf("%s", menu_title);

    set_cursor(0,TERM_POS_QUES_S);
    for (int i=0;i<w.ws_col;i++)
        printf("#");

    set_cursor(((w.ws_col/2) - (l_question/2)),TERM_POS_QUES_S);
    printf("%s", question_title);

    set_cursor(0,TERM_POS_INPT_S);
    for (int i=0;i<w.ws_col;i++)
        printf("#");

    set_cursor(((w.ws_col/2) - (l_input/2)),TERM_POS_INPT_S);
    printf("%s", input_title);

    set_cursor(0,TERM_POS_DEBG_S);
    for (int i=0;i<w.ws_col;i++)
        printf("#");

    set_cursor(0,TERM_POS_DEBG_S+1);
        printf(" # | SCOPE | COMP | LVL | MESSAGE");

    set_cursor(((w.ws_col/2) - (l_debug/2)),TERM_POS_DEBG_S);
    printf("%s", debug_title);

    set_cursor(0,TERM_POS_DEBG_E);
    for (int i=0;i<w.ws_col;i++)
        printf("#");

    fflush(stdout);

    term_unlock();
}

void g_main_menu(void)
{
    term_lock();
    set_cursor(0,TERM_POS_MENU_S+1);
    clrline();printf("-Mód kalkulačky\n");
    clrline();printf("   Výpočet (D)ojazdu\n");
    clrline();printf("   Výpočet potrebného/spotrebovaného (P)aliva\n");
    clrline();printf("\n");
    clrline();printf("-Iné\n");
    clrline();printf("   (E)xit program\n");
    clrline();printf("\n");
    clrline();printf("\n");
    clrline();printf("\n");
    clrline();
    set_cursor(0,TERM_POS_QUES_S+1);
    clrline();printf("Pre výber z menu zadajte zvýraznené písmeno a potvrďte Enterom.\n");
    fflush(stdout);
    set_cursor(3,TERM_POS_INPT);
    term_unlock();
}

void g_menu1(void)
{
    term_lock();
    set_cursor(0,TERM_POS_MENU_S+1);
    clrline();printf("-Zadajte potrebné dáta\n");
    clrline();printf("   Aktuálne/Spotrebované palivo [l]\n");
    clrline();printf("   Priemerná spotreba paliva [l/100km]\n");
    clrline();printf("   *Cena paliva [eur/l]\n");
    clrline();printf("   *Počet cestujúcich [#]\n");
    clrline();printf("-*Voliteľný parameter - Pre ignorovanie stlačte ENTER.\n");
    clrline();printf("\n");
    clrline();printf("-Iné\n");
    clrline();printf("   (R)eturn - Návrat na predošlé menu.\n");
    clrline();printf("   (E)xit program\n");
    fflush(stdout);
    set_cursor(3,TERM_POS_INPT);
    term_unlock();
}

void g_menu2(void)
{
    term_lock();
    set_cursor(0,TERM_POS_MENU_S+1);
    clrline();printf("-Zadajte potrebné dáta\n");
    clrline();printf("   Vzdialenosť [km]\n");
    clrline();printf("   Priemerná spotreba paliva [l/100km]\n");
    clrline();printf("   *Cena paliva [eur/l]\n");
    clrline();printf("   *Počet cestujúcich [#]\n");
    clrline();printf("-*Voliteľný parameter - Pre ignorovanie stlačte ENTER.\n");
    clrline();printf("\n");
    clrline();printf("-Iné\n");
    clrline();printf("   (R)eturn - Návrat na predošlé menu.\n");
    clrline();printf("   (E)xit program\n");
    fflush(stdout);
    set_cursor(3,TERM_POS_INPT);
    term_unlock();
}

void g_table1(void)
{
    term_lock();
    set_cursor(0,TERM_POS_MENU_S+1);
    clrline();printf("-Výstup kalkulačky\n");
    clrline();printf("┌───────────────╥────────────┬──────────────┬─────────────────┬───────────┐\n");
    clrline();printf("│               ║   Palivo   │   Spotreba   │ Jednotkova cena │ Cestujúci │\n");
    clrline();printf("├───────────────╫────────────┼──────────────┼─────────────────┼───────────┤\n");
    //                000000000011111111112222222222333333333334444444445555555555666666666677777
    //                012345678901234567890123456789012345678901234567890123456789012345678901234
    clrline();printf("│ Vstupné dáta  ║            │              │                 │           │\n");
    clrline();printf("├───────────────╫────────────┼──────────────┼─────────────────┴───────────┤\n");
    clrline();printf("│               ║   Dojazd   │ Celkova cena │     Cena na jednotlivca     │\n");
    clrline();printf("├───────────────╫────────────┼──────────────┼─────────────────────────────┤\n");
    //                000000000011111111112222222222333333333334444444445555555555666666666677777
    //                012345678901234567890123456789012345678901234567890123456789012345678901234
    clrline();printf("│ Výstupné dáta ║            │              │                             │\n");
    clrline();printf("└───────────────╨────────────┴──────────────┴─────────────────────────────┘\n");
    fflush(stdout);
    set_cursor(3,TERM_POS_INPT);
    term_unlock();
}

void g_table2(void)
{
    term_lock();
    set_cursor(0,TERM_POS_MENU_S+1);
    clrline();printf("-Výstup kalkulačky\n");
    clrline();printf("┌───────────────╥────────────┬──────────────┬─────────────────┬───────────┐\n");
    clrline();printf("│               ║ Vzdialenosť│   Spotreba   │ Jednotkova cena │ Cestujúci │\n");
    clrline();printf("├───────────────╫────────────┼──────────────┼─────────────────┼───────────┤\n");
    clrline();printf("│ Vstupné dáta  ║            │              │                 │           │\n");
    clrline();printf("├───────────────╫────────────┼──────────────┼─────────────────┴───────────┤\n");
    clrline();printf("│               ║   Palivo   │ Celkova cena │     Cena na jednotlivca     │\n");
    clrline();printf("├───────────────╫────────────┼──────────────┼─────────────────────────────┤\n");
    clrline();printf("│ Výstupné dáta ║            │              │                             │\n");
    clrline();printf("└───────────────╨────────────┴──────────────┴─────────────────────────────┘\n");
    fflush(stdout);
    set_cursor(3,TERM_POS_INPT);
    term_unlock();
}

void g_ending(void)
{
    term_lock();
    set_cursor(0,TERM_POS_MENU_S+1);
    clrline();printf("-Zatvorenie aplikacie\n");
    clrline();printf("\n");
    clrline();printf("-Credits\n");
    clrline();printf(" AUTHOR: Marek Sykorka\n");
    clrline();printf(" AIS ID: 115025\n");
    clrline();printf(" Hope you enjoyed using my program :)\n");
    clrline();printf("\n");
    clrline();printf("\n");
    clrline();printf("\n");
    clrline();printf("\n");
    fflush(stdout);
    set_cursor(3,TERM_POS_INPT);
    term_unlock();
}

// Klient 2
int client2_main(void)
{
    // Local Variable Defines
    SOCKET_DATA in_data, out_data;
    CL2_OBJ cl2_closable_obj;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;

    sprintf(debug_msg.mesg_text, "{CHLD} [PROC]  (i)   2:%d alive.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    
    // Create Process Termination Thread
    sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (i)   2:%d Creating termination thread.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    pthread_t dstr_thrd;
    pthread_create(&dstr_thrd,NULL,thr_terminateCheck2,&cl2_closable_obj);

    // Wait for Initialization signal to arrive from server
    if(waitForSig(SIGINIT))
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [PROC]  (i)   2:%d Init complete. Initialization signal arrived. Child starting.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    }

    // Create client socket
    int d_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    cl2_closable_obj.client_socket = d_client_socket;
    // Check creation status
    if (d_client_socket == -1)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   2:%d Cannot create client socket.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client2_error = errno;
        kill(getppid(),SIGTERMINATE);
    }
    // Socket creation succesful
    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   2:%d Client socket %d created.",getpid(),d_client_socket);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Setting socket
    struct sockaddr_in client;  
    memset(&client, 0, sizeof(client));  
    client.sin_family = AF_INET;  
    client.sin_addr.s_addr = inet_addr("127.0.0.1");  
    client.sin_port = htons(SOCK_PORT);

     // Connecting to client with created socket
    if (connect(d_client_socket, (struct sockaddr*)&client, sizeof(client)) != 0)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   2:%d Cannot connect to 127.0.0.1:%d.",getpid(),SOCK_PORT);
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client2_error = errno;
        kill(getppid(),SIGTERMINATE);
    }

    // Socket connection succesful
    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   2:%d Connected to 127.0.0.1:%d.",getpid(),SOCK_PORT);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   2:%d Waiting for AUTH request.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    while(in_data.control_word != SOCK_AUTH_REQ)
    {
        if (read(d_client_socket, &in_data, sizeof(in_data)) < 0)
        {
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   2:%d Socket FAILED.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            client2_error = errno;
            kill(getppid(),SIGTERMINATE);     
        }
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (>)   2:%d AUTH request received.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

        if(in_data.control_word == SOCK_AUTH_REQ)
        {
            out_data.control_word = P_CLI2;
            if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   2:%d Socket FAILED.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client2_error = errno;
                kill(getppid(),SIGTERMINATE);       
            }
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (<)   2:%d AUTH message sent.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            
            if (read(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   2:%d Socket FAILED.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client2_error = errno;
                kill(getppid(),SIGTERMINATE);
            }

            if(out_data.control_word == SOCK_ACKW)
            {     
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   2:%d AUTH succesful ACKW received.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            }
            else
            {     
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   2:%d AUTH not succesful.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client2_error = EINVAL;
                kill(getppid(),SIGTERMINATE);
            }
        }
    }

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    char debug_title_paused[] = " Debug Info - PAUSED ";
    int l_debug_paused = strlen(debug_title_paused);
    char debug_title[] = " Debug Info ";
    int l_debug = strlen(debug_title);

    while(1)
    {
        memset(&in_data, 0, sizeof(in_data));
        memset(&out_data, 0, sizeof(out_data));

        // Await instructions from server
        if (read(d_client_socket, &in_data, sizeof(in_data)) < 0)
        {
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   %d Socket FAILED.\n",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            // ERROR socket can not communicate - terminate app
            kill(getppid(),SIGTERMINATE);          
        }

        if(in_data.control_word == SOCK_DATA_REQ)
        {
            //Lock terminal acces
            term_lock();
            //State that debug info is being paused for input
            set_cursor(0,TERM_POS_DEBG_S);
            for (int i=0;i<w.ws_col;i++)
                printf("#");
            set_cursor(((w.ws_col/2) - (l_debug_paused/2)),TERM_POS_DEBG_S);
            printf("%s", debug_title_paused);
            //Input set cursor
            set_cursor(0,TERM_POS_INPT);
            clrline();
            printf(">>");
            fflush(stdout);
            //Awaiting input
            fgets(out_data.simple_data.s_data, sizeof(out_data.simple_data.s_data), stdin);
            //Try parse input into diffrent data types
            out_data.simple_data.c_data = out_data.simple_data.s_data[0];
            out_data.simple_data.i_data = atoi(out_data.simple_data.s_data);
            out_data.simple_data.f_data = atof(out_data.simple_data.s_data);
            if(out_data.simple_data.c_data == '\n')
                out_data.simple_data.c_data = '-';
            sprintf(debug_msg.mesg_text, "{CHLD} [INPT]  (i)   2:%d Processed input, char: %c int: %i float: %f\n",getpid(),out_data.simple_data.c_data,out_data.simple_data.i_data,out_data.simple_data.f_data);
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            //State that debug info is resumed
            set_cursor(0,TERM_POS_DEBG_S);
            for (int i=0;i<w.ws_col;i++)
                printf("#");
            set_cursor(((w.ws_col/2) - (l_debug/2)),TERM_POS_DEBG_S);
            printf("%s", debug_title);
            fflush(stdout);
            //prepare data
            out_data.control_word = SOCK_ACKW;
            out_data.destination = in_data.source;
            out_data.source = in_data.destination;

            if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   2:%d Socket FAILED.\n",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client2_error = errno;
                kill(getppid(),SIGTERMINATE);          
            }

            //unlock terminal acces
            term_unlock();
        }
    }
}

// Klient 3
int client3_main(void)
{
    // Local Variable Defines
    SOCKET_DATA in_data, out_data;
    CL3_OBJ cl3_closable_obj;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;

    sprintf(debug_msg.mesg_text, "{CHLD} [PROC]  (i)   3:%d alive.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Create Process Termination Thread
    sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (i)   3:%d Creating termination thread.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    pthread_t dstr_thrd;
    pthread_create(&dstr_thrd,NULL,thr_terminateCheck3,&cl3_closable_obj);

    // Wait for Initialization signal to arrive from server
    if(waitForSig(SIGINIT))
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [PROC]  (i)   3:%d Init complete. Initialization signal arrived. Child starting.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    }

    // Create client socket
    int d_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    cl3_closable_obj.client_socket = d_client_socket;
    // Check creation status
    if (d_client_socket == -1)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Cannot create client socket.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client3_error = errno;
        kill(getppid(),SIGTERMINATE);
    }
    // Socket creation succesful
    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   3:%d Client socket %d created.",getpid(),d_client_socket);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Setting socket
    struct sockaddr_in client;  
    memset(&client, 0, sizeof(client));  
    client.sin_family = AF_INET;  
    client.sin_addr.s_addr = inet_addr("127.0.0.1");  
    client.sin_port = htons(SOCK_PORT);

     // Connecting to client with created socket
    if (connect(d_client_socket, (struct sockaddr*)&client, sizeof(client)) != 0)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Cannot connect to 127.0.0.1:%d.",getpid(),SOCK_PORT);
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client3_error = errno;
        kill(getppid(),SIGTERMINATE);
    }

    // Socket connection succesful
    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   3:%d Connected to 127.0.0.1:%d.",getpid(),SOCK_PORT);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   3:%d Waiting for AUTH request.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    while(in_data.control_word != SOCK_AUTH_REQ)
    {
        if (read(d_client_socket, &in_data, sizeof(in_data)) < 0)
        {
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Socket FAILED.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            client3_error = errno;
            kill(getppid(),SIGTERMINATE);     
        }
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (>)   3:%d AUTH request received.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

        if(in_data.control_word == SOCK_AUTH_REQ)
        {
            out_data.control_word = P_CLI3;
            if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Socket FAILED.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client3_error = errno;
                kill(getppid(),SIGTERMINATE);       
            }
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (<)   3:%d AUTH message sent.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            
            if (read(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Socket FAILED.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client3_error = errno;
                kill(getppid(),SIGTERMINATE);
            }

            if(out_data.control_word == SOCK_ACKW)
            {     
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   3:%d AUTH succesful ACKW received.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            }
            else
            {     
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   3:%d AUTH not succesful.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client3_error = EINVAL;
                kill(getppid(),SIGTERMINATE);
            }
        }
    }

    // Client 3 main loop
    while(1)
    {
        // Every loop clear all data structs
        memset(&in_data, 0, sizeof(in_data));
        memset(&out_data, 0, sizeof(out_data));

        // Await instructions from server
        if (read(d_client_socket, &in_data, sizeof(in_data)) < 0)
        {
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Socket FAILED.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            client3_error = errno;
            kill(getppid(),SIGTERMINATE);       
        }

        switch (in_data.data.calculator_mode)
        {
            // Calculator mode 1
            case CALC_MODE1:
            {
                out_data = in_data;

                // Vypocty / vypoctoveho procesu
                if(in_data.data.spotreba != 0)
                {
                    out_data.data.dojazd = ((in_data.data.palivo / in_data.data.spotreba)*100);
                }
                else
                {
                    out_data.data.dojazd = -1;
                }

                if(in_data.data.jednotkova_cena_paliva != 0)
                {
                    out_data.data.cena_paliva = in_data.data.palivo*in_data.data.jednotkova_cena_paliva;
                    if(in_data.data.ludia != 0)
                    {
                        out_data.data.cena_na_cloveka = out_data.data.cena_paliva/in_data.data.ludia;
                    }
                    else
                    {
                        out_data.data.cena_na_cloveka = -1;
                    }
                }
                else
                {
                    out_data.data.cena_paliva = -1;
                }

                out_data.destination = in_data.source;
                out_data.source = in_data.destination;
                out_data.control_word = CALC_MODE1;

                if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
                {
                    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Socket FAILED.",getpid());
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    client3_error = errno;
                    kill(getppid(),SIGTERMINATE);   
                }

                break;
            }
            // Calculator mode 2
            case CALC_MODE2:
            {
                out_data = in_data;

                // Vypocty / vypoctoveho procesu
                if(in_data.data.spotreba != 0)
                {
                    out_data.data.potrebne_palivo = ((in_data.data.vzdialenost / 100)*in_data.data.spotreba);
                }
                else
                {
                    out_data.data.potrebne_palivo = -1;
                }

                if(in_data.data.jednotkova_cena_paliva != 0)
                {
                    out_data.data.cena_paliva = out_data.data.potrebne_palivo*in_data.data.jednotkova_cena_paliva;
                    if(in_data.data.ludia != 0)
                    {
                        out_data.data.cena_na_cloveka = out_data.data.cena_paliva/in_data.data.ludia;
                    }
                    else
                    {
                        out_data.data.cena_na_cloveka = -1;
                    }
                }
                else
                {
                    out_data.data.cena_paliva = -1;
                }               

                out_data.destination = in_data.source;
                out_data.source = in_data.destination;
                out_data.control_word = CALC_MODE2;

                if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
                {
                    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Socket FAILED.",getpid());
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    client3_error = errno;
                    kill(getppid(),SIGTERMINATE);   
                }

                break;
            }
            default:
            {
                out_data = in_data;
                out_data.destination = in_data.source;
                out_data.source = in_data.destination;
                out_data.control_word = SOCK_FAIL;

                if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
                {
                    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   3:%d Socket FAILED.",getpid());
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    client3_error = EINVAL;
                    kill(getppid(),SIGTERMINATE);   
                }
            }
        }
    }
}

// Klient 4
int client4_main(void)
{
    // Local Variable Defines
    SOCKET_DATA in_data, out_data;
    CL4_OBJ cl4_closable_obj;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;
    FILE *save_fd = NULL;
    cl4_closable_obj.out_fd = save_fd;

    sprintf(debug_msg.mesg_text, "{CHLD} [PROC]  (i)   4:%d alive.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    
    // Create Process Termination Thread
    sprintf(debug_msg.mesg_text, "{CHLD} [THRD]  (i)   4:%d Creating termination thread.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    pthread_t dstr_thrd;
    pthread_create(&dstr_thrd,NULL,thr_terminateCheck4,&cl4_closable_obj);   
    
    // Wait for Initialization signal to arrive from server
    if(waitForSig(SIGINIT))
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [PROC]  (i)   4:%d Init complete. Initialization signal arrived. Child starting.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
    }

    // Create client socket
    int d_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    cl4_closable_obj.client_socket = d_client_socket;
    // Check creation status
    if (d_client_socket == -1)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   4:%d Cannot create client socket.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client4_error = errno;
        kill(getppid(),SIGTERMINATE);
    }
    // Socket creation succesful
    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   4:%d Client socket %d created.",getpid(),d_client_socket);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    // Setting socket
    struct sockaddr_in client;  
    memset(&client, 0, sizeof(client));  
    client.sin_family = AF_INET;  
    client.sin_addr.s_addr = inet_addr("127.0.0.1");  
    client.sin_port = htons(SOCK_PORT);

     // Connecting to client with created socket
    if (connect(d_client_socket, (struct sockaddr*)&client, sizeof(client)) != 0)
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   4:%d Cannot connect to 127.0.0.1:%d.",getpid(),SOCK_PORT);
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        client4_error = errno;
        kill(getppid(),SIGTERMINATE);
    }

    // Socket connection succesful
    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   4:%d Connected to 127.0.0.1:%d.",getpid(),SOCK_PORT);
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   4:%d Waiting for AUTH request.",getpid());
    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

    while(in_data.control_word != SOCK_AUTH_REQ)
    {
        if (read(d_client_socket, &in_data, sizeof(in_data)) < 0)
        {
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   4:%d Socket FAILED.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            client4_error = errno;
            kill(getppid(),SIGTERMINATE);     
        }
        sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (>)   4:%d AUTH request received.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

        if(in_data.control_word == SOCK_AUTH_REQ)
        {
            out_data.control_word = P_CLI4;
            if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   4:%d Socket FAILED.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client4_error = errno;
                kill(getppid(),SIGTERMINATE);       
            }
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (<)   4:%d AUTH message sent.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            
            if (read(d_client_socket, &out_data, sizeof(out_data)) < 0)
            {
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   4:%d Socket FAILED.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client4_error = errno;
                kill(getppid(),SIGTERMINATE);
            }

            if(out_data.control_word == SOCK_ACKW)
            {     
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   4:%d AUTH succesful ACKW received.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            }
            else
            {     
                sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (i)   4:%d AUTH not succesful.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                client4_error = EINVAL;
                kill(getppid(),SIGTERMINATE);
            }
        }
    }

    // Client 4 main loop
    while(1)
    {
        // Every loop clear all data structs
        memset(&in_data, 0, sizeof(in_data));
        memset(&out_data, 0, sizeof(out_data));

        // Await instructions from server
        if (read(d_client_socket, &in_data, sizeof(in_data)) < 0)
        {
            sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   4:%d Socket FAILED.",getpid());
            msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
            client4_error = errno;
            kill(getppid(),SIGTERMINATE); 
        }

        switch (in_data.control_word)
        {
            case TERM_FILEEXISTS_MENU:
            {
                out_data = in_data;

                // Existuje subor ?
                if(save_fd == NULL)
                {
                    // Nie vypytaj si nazov suboru
                    out_data.simple_data.i_data = 0;
                    sprintf(debug_msg.mesg_text, "{CHLD} [FILE]  (i)   4:%d File does not exist.",getpid());
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                }
                else
                {
                    // Ano pis do suboru
                    out_data.simple_data.i_data = 1;
                    sprintf(debug_msg.mesg_text, "{CHLD} [FILE]  (i)   4:%d File exists.",getpid());
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                }

                out_data.destination = in_data.source;
                out_data.source = in_data.destination;
                out_data.control_word = TERM_FILEEXISTS_MENU;

                if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
                {
                    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   4:%d Socket FAILED.",getpid());
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    client4_error = errno;
                    kill(getppid(),SIGTERMINATE);         
                }

                break;
            }
            case TERM_WRITEFILE_MENU:
            {
                out_data = in_data;

                if(save_fd == NULL)
                {
                    save_fd = fopen(in_data.simple_data.s_data,"w+");
                    cl4_closable_obj.out_fd = save_fd;

                    if(save_fd == NULL)
                    {
                        sprintf(debug_msg.mesg_text, "{CHLD} [FILE]  (!)   4:%d Error creating file.",getpid());
                        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                        client4_error = errno;
                        kill(getppid(),SIGTERMINATE);    
                        break; 
                    }
                    else
                    {
                        sprintf(debug_msg.mesg_text, "{CHLD} [FILE]  (i)   4:%d New file created with descriptor %p.",getpid(),save_fd);
                        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    }

                    sprintf(debug_msg.mesg_text, "{CHLD} [FILE]  (i)   4:%d Writing header to file.",getpid());
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    fprintf(save_fd,"INPUT,Palivo,Vzdialenosť,Spotreba,*Jednotková cena,*Počet cestujúcich,OUTPUT,Palivo,Dojazd,Celková cena, Cena na jednotlivca\n");
                    fprintf(save_fd,"Test");
                }
                
                sprintf(debug_msg.mesg_text, "{CHLD} [FILE]  (i)   4:%d Writing to file started.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

                // Input
                fprintf(save_fd,"INPUT,");
                // Palivo
                //---------------------------------------------------------
                if(in_data.data.palivo == 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%.2f", in_data.data.palivo);

                fprintf(save_fd,",");
                //---------------------------------------------------------
                if(in_data.data.vzdialenost == 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%.2f", in_data.data.vzdialenost);

                fprintf(save_fd,",");
                //---------------------------------------------------------   
                if(in_data.data.spotreba == 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%.2f", in_data.data.spotreba);

                fprintf(save_fd,",");
                //---------------------------------------------------------
                if(in_data.data.jednotkova_cena_paliva == 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%.2f", in_data.data.jednotkova_cena_paliva);

                fprintf(save_fd,",");
                //---------------------------------------------------------
                if(in_data.data.ludia == 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%d", in_data.data.ludia);

                fprintf(save_fd,",");
                //Output
                fprintf(save_fd,"OUTPUT,");
                //---------------------------------------------------------
                if(in_data.data.potrebne_palivo <= 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%.2f", in_data.data.potrebne_palivo);

                fprintf(save_fd,",");
                //---------------------------------------------------------
                if(in_data.data.dojazd <= 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%.2f", in_data.data.dojazd);

                fprintf(save_fd,",");
                //---------------------------------------------------------
                if(in_data.data.cena_paliva <= 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%.2f", in_data.data.cena_paliva);

                fprintf(save_fd,",");
                //---------------------------------------------------------
                if(in_data.data.cena_na_cloveka <= 0)
                    fprintf(save_fd,TERM_TABLE_NA);
                else
                    fprintf(save_fd,"%.2f", in_data.data.cena_na_cloveka);

                fprintf(save_fd,"\n");
                //---------------------------------------------------------

                sprintf(debug_msg.mesg_text, "{CHLD} [FILE]  (i)   4:%d Writing to file ended.",getpid());
                msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);

                out_data.destination = in_data.source;
                out_data.source = in_data.destination;
                out_data.control_word = SOCK_ACKW;

                if (write(d_client_socket, &out_data, sizeof(out_data)) < 0)
                {
                    sprintf(debug_msg.mesg_text, "{CHLD} [SOCK]  (!)   4:%d Socket FAILED.",getpid());
                    msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
                    client4_error = errno;
                    kill(getppid(),SIGTERMINATE);            
                }
                break;
            }            
        }
    }
}

// Ostatné
int waitForSig(int __signo)
{
    int sig;
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, __signo);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
 
    if(sigwait(&sigset, &sig) == 0)
        return sig;
}

void *thr_terminateCheck1(void *args)
{
    CL1_OBJ *actual_args = args;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;

    while(1)
    {
        if(check_terminate_client())
        {
            pthread_cancel(actual_args->graph_thred);
            set_cursor(0,0);cls();
            printf("--- TERMINATION INFO ---\n\n");
            printf("{CHLD1} [EXIT]  (‼)   GRAPHIC THREAD TERMINATED.\n");
            printf("PRIMITIVE GRAPHIC OUTPUT...\n\n");

            if(actual_args->client_socket < 0)
                printf("{CHLD1} [EXIT]  (‼)   CLIENT-SIDE SOCKET 1 DOES NOT EXIST.\n");
            else
                printf("{CHLD1} [EXIT]  (‼)   CLOSING CLIENT-SIDE SOCKET 1 SCK_DSC:%d with status '%d'.\n",actual_args->client_socket ,close(actual_args->client_socket));

            printf("{CHLD1} [EXIT]  (‼)   CLOSING PIPE FD[0]:%d with status '%d'.\n",actual_args->ithr_pipe_str.pipe_fd[0],close(actual_args->ithr_pipe_str.pipe_fd[0]));
            printf("{CHLD1} [EXIT]  (‼)   CLOSING PIPE FD[1]:%d with status '%d'.\n",actual_args->ithr_pipe_str.pipe_fd[1],close(actual_args->ithr_pipe_str.pipe_fd[1]));

            printf("{CHLD1} [EXIT]  (‼)   EXITING. %d -> %s\n",client1_error,strerror(client1_error));
            fflush(stdout);

            exit(client1_error);
        }
    }
}

void *thr_terminateCheck2(void *args)
{
    CL2_OBJ *actual_args = args;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;

    while(1)
    {
        if(check_terminate_client())
        {
            if(actual_args->client_socket < 0)
                printf("{CHLD2} [EXIT]  (‼)   CLIENT-SIDE SOCKET 2 DOES NOT EXIST.\n");
            else
                printf("{CHLD2} [EXIT]  (‼)   CLOSING CLIENT-SIDE SOCKET 2 SCK_DSC:%d with status '%d'.\n",actual_args->client_socket ,close(actual_args->client_socket));

            printf("{CHLD2} [EXIT]  (‼)   EXITING. %d -> %s\n",client2_error,strerror(client2_error));
            fflush(stdout);

            exit(client2_error);
        }
    }
}

void *thr_terminateCheck3(void *args)
{
    CL3_OBJ *actual_args = args;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;

    while(1)
    {
        if(check_terminate_client())
        {
            if(actual_args->client_socket < 0)
                printf("{CHLD3} [EXIT]  (‼)   CLIENT-SIDE SOCKET 3 DOES NOT EXIST.\n");
            else
                printf("{CHLD3} [EXIT]  (‼)   CLOSING CLIENT-SIDE SOCKET 3 SCK_DSC:%d with status '%d'.\n",actual_args->client_socket ,close(actual_args->client_socket));

            printf("{CHLD3} [EXIT]  (‼)   EXITING. %d -> %s\n",client3_error,strerror(client3_error));
            fflush(stdout);

            exit(client3_error);
        }
    }
}

void *thr_terminateCheck4(void *args)
{
    CL4_OBJ *actual_args = args;
    DEBUG_MESSAGE debug_msg; debug_msg.mesg_type = 1;

    while(1)
    {
        if(check_terminate_client())
        {
            if(actual_args->client_socket < 0)
                printf("{CHLD4} [EXIT]  (‼)   CLIENT-SIDE SOCKET 4 DOES NOT EXIST.\n");
            else
                printf("{CHLD4} [EXIT]  (‼)   CLOSING CLIENT-SIDE SOCKET 4 SCK_DSC:%d with status '%d'.\n",actual_args->client_socket ,close(actual_args->client_socket));

            if(actual_args->out_fd == NULL)
                printf("{CHLD4} [EXIT]  (‼)   OUTPUT FILE:%p DOES NOT EXIST\n",actual_args->out_fd);
            else
                printf("{CHLD4} [EXIT]  (‼)   CLOSING OUTPUT FILE:%p with status '%d'\n",actual_args->out_fd,fclose(actual_args->out_fd));

            printf("{CHLD4} [EXIT]  (‼)   EXITING. %d -> %s\n",client4_error,strerror(client4_error));
            fflush(stdout);

            exit(client4_error);
        }
    }
}

int check_terminate_client()
{
    DEBUG_MESSAGE debug_msg;
    debug_msg.mesg_type = 1;

    sigset_t pending;
    sigpending(&pending);
    if (sigismember(&pending, SIGTERMINATE))
    {
        sprintf(debug_msg.mesg_text, "{CHLD} [EXIT]  (‼)   %d Termination called.",getpid());
        msgsnd(debug_info_ms_id,&debug_msg,sizeof(debug_msg), 0);
        return 1;
    }
    return 0;
}
