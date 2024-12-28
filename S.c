#include "header.h"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>  
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/mman.h>  
#include <semaphore.h>
#include <unistd.h>  
#include <fcntl.h>    
#include <stdbool.h>
#include <dirent.h>
#include <sys/wait.h>
#include <time.h>


//ALL THE STRUCTS N GLOBAL VARS DEFINED HERE 
struct Nodes
{
    struct Nodes *next;
    int priority;
    char fname[20];
    pid_t pid;
    double t_exec;
    double t_wait;
    double waiting_process;
    double turn_around_time;
};



struct Proccess_Type
{
    struct Nodes *front;
    struct Nodes *back;
};


struct Nodes *node_add(pid_t pid, char *fname, int priority, double exec, double wait, double waiting_priorityocess,double turn_around_time)
{
    struct Nodes *curr = (struct Nodes *)malloc(sizeof(struct Nodes));
    curr->t_exec = exec;
    curr->t_wait = wait;
    curr->waiting_process = waiting_priorityocess;
    strcpy(curr->fname, fname);
    curr->turn_around_time = turn_around_time;
    curr->pid = pid;
    curr->next = NULL;
    curr->priority = priority;
    return curr;
}

typedef struct {
    struct Proccess_Type High_P_Queue;
    struct Proccess_Type New_Queue;
    struct Proccess_Type Ready_Queue;
    struct Proccess_Type Initial_Queue;
    sem_t mutex;
} Sharedd_Memory;

int NCPU;
float TSLICE;

struct Proccess_Type *High_P_Queue;
struct Proccess_Type *New_Queue;
volatile sig_atomic_t Semaphore_Signal = 0;
struct Proccess_Type *Initial_Queue;
struct Proccess_Type *Ready_Queue;
int Shared_Memory_Segment;
Sharedd_Memory* SHM;



/////////////////////////////////SIGNAL HANDLING USR1 AND USR2


void Signal_Catching(int signum)
{
    if (signum == SIGUSR1)
    {
        printf("\n Running the processes \n");
        Semaphore_Signal = 1;
    }
    else if (signum == SIGUSR2)
    {
        if (Initial_Queue->front == NULL)
        {
            printf("Scheduler Terminated\n");
            Scheduler_History(High_P_Queue);
            exit(0);
        }
        else
        {
            printf("\nScheduler hasn't finished all the processes \n");
            Semaphore_Signal = 1;
        }

        while (Semaphore_Signal)
        {
            break;
        }

        Scheduler_History(High_P_Queue);

        munmap(SHM, sizeof(Sharedd_Memory));
        close(Shared_Memory_Segment);
        shm_unlink("/my_shm");
        sem_destroy(&SHM->mutex);
        exit(0);
    }
}


void SIGINT_CATCH(int signum) {
    if (signum == SIGINT) {
        printf("TERMINATED");

        if (Initial_Queue->front != NULL)
        {
            printf("\nScheduler hasnt finished all the processes \n");
            Semaphore_Signal = 1;
        }
        else
        {
            printf("Scheduler Terminated \n");
        }

        while (1)
        {
            if (!Semaphore_Signal)
            {
                break;
            }
        }

        Scheduler_History(High_P_Queue);

        munmap(SHM, sizeof(Sharedd_Memory));
        close(Shared_Memory_Segment);
        shm_unlink("/my_shm");
        sem_destroy(&SHM->mutex);
        exit(0);
    }
}

/////////////////////////// MAKING A QUEUE 

struct Proccess_Type *Init_Queue()
{
    struct Proccess_Type *curr = (struct Proccess_Type *) malloc(sizeof(struct Proccess_Type)+sizeof(struct Nodes));
    curr->front = curr->back = NULL;


    return curr;
}

void End_process(struct Proccess_Type *Initial_Queue)
{
    if (Initial_Queue->front == NULL)
    {
        return;
    }
    else if (Initial_Queue->front->next == NULL)
    {
        struct Nodes *curr = Initial_Queue->front;
        Initial_Queue->front = Initial_Queue->back = NULL;
        free(curr);
    }
    else
    {
        struct Nodes *curr = Initial_Queue->front;
        Initial_Queue->front = Initial_Queue->front->next;
        free(curr);
    }
}

/////////////////////////////////////////////////QUEINING METHODS ALL 


void Enqueue_Proc(struct Proccess_Type *Initial_Queue, pid_t pid, char *fname, int priority, double exec, double wait, double waiting_process,double turn_around_time)
{
    struct Nodes *curr = node_add(pid, fname, priority, exec, wait, waiting_process,turn_around_time);
    if (Initial_Queue->back == NULL)
    {
        Initial_Queue->front = Initial_Queue->back = curr;
        return;
    }

    Initial_Queue->back->next = curr;
    Initial_Queue->back = curr;
}


void Front_Enqueue_Priority(struct Proccess_Type *Initial_Queue, pid_t pid, char *fname, int priority, 
                            double exec, double wait, double waiting_process, double turn_around_time)
{
    struct Nodes *curr = node_add(pid, fname, priority, exec, wait, waiting_process, turn_around_time);
    
    if (priority > 4)
    {
        printf("\n Priority : valid range is [1-4].\n");
        return;
    }

    if (Initial_Queue->back == NULL)
    {
        Initial_Queue->front = curr;
        Initial_Queue->back = curr;
        return;
    }

    struct Nodes *in_front = Initial_Queue->front;
    struct Nodes *in_behind = Initial_Queue->front;

    while (in_behind != NULL)
    {
        if (priority == 1)
        {
            Initial_Queue->front = curr;
            Initial_Queue->front->next = in_behind;
            break;
        }
        else if (priority == 2)
        {
            if (in_front != NULL)
            {
                if (in_front->priority >= 2 && Initial_Queue->front == in_front)
                {
                    Initial_Queue->front = curr;
                    Initial_Queue->front->next = in_front;
                    break;
                }
                else if (in_front->priority >= 2 && in_behind->priority <= 1)
                {
                    in_behind->next = curr;
                    curr->next = in_front;
                    break;
                }
            }
            else if (in_front == NULL && in_behind->priority <= 1)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
        }
        else if (priority == 3)
        {
            if (in_front != NULL)
            {
                if (Initial_Queue->front == in_front && in_front->priority >= 3)
                {
                    Initial_Queue->front = curr;
                    Initial_Queue->front->next = in_front;
                    break;
                }
                else if (in_behind->priority <= 2 && in_front->priority >= 3)
                {
                    in_behind->next = curr;
                    curr->next = in_front;
                    break;
                }
            }
            else if (in_front == NULL && in_behind->priority <= 2)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
        }
        else if (priority == 4)
        {
            if (in_front != NULL)
            {
                if (Initial_Queue->front == in_front && in_front->priority >= 4)
                {
                    Initial_Queue->front = curr;
                    Initial_Queue->front->next = in_front;
                    break;
                }
                else if (in_behind->priority <= 3 && in_front->priority >= 4)
                {
                    in_behind->next = curr;
                    curr->next = in_front;
                    break;
                }
            }
            else if (in_front == NULL && in_behind->priority <= 3)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
        }

        in_behind = in_front;
        in_front = in_front->next;
    }
}
void End_Enqueue_Priority(struct Proccess_Type *Initial_Queue, pid_t pid, char *fname, int priority, 
                          double exec, double wait, double waiting_process, double turn_around_time)
{
    struct Nodes *curr = node_add(pid, fname, priority, exec, wait, waiting_process, turn_around_time);

    if (priority > 4)
    {
        printf("Priority's valid range is (1-4)\n");
        return;
    }

    if (Initial_Queue->back == NULL)
    {
        Initial_Queue->front = Initial_Queue->back = curr;
        return;
    }

    struct Nodes *in_front = Initial_Queue->front;
    struct Nodes *in_behind = Initial_Queue->front;

    while (in_behind != NULL)
    {
        if (priority < 1 || priority > 4)
        {
            printf("Priority's valid range is (1-4)\n");
            break;
        }

        if (priority == 1)
        {
            if (in_front != NULL && in_front->priority >= 2 && Initial_Queue->front == in_front)
            {
                Initial_Queue->front = curr;
                Initial_Queue->front->next = in_front;
                break;
            }
            else if (in_front != NULL && in_behind->priority <= 1 && in_front->priority >= 2)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
            else if (in_front == NULL && in_behind->priority <= 1)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
        }

        else if (priority == 2)
        {
            if (in_front != NULL && in_front->priority >= 3 && Initial_Queue->front == in_front)
            {
                Initial_Queue->front = curr;
                Initial_Queue->front->next = in_front;
                break;
            }
            else if (in_front != NULL && in_behind->priority <= 2 && in_front->priority >= 3)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
            else if (in_front == NULL && in_behind->priority <= 2)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
        }

        else if (priority == 3)
        {
            if (in_front != NULL && in_front->priority >= 4 && Initial_Queue->front == in_front)
            {
                Initial_Queue->front = curr;
                Initial_Queue->front->next = in_front;
                break;
            }
            else if (in_front != NULL && in_behind->priority <= 3 && in_front->priority >= 4)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
            else if (in_front == NULL && in_behind->priority <= 3)
            {
                in_behind->next = curr;
                curr->next = in_front;
                break;
            }
        }

        else if (priority == 4 && in_front == NULL)
        {
            in_behind->next = curr;
            curr->next = in_front;
            break;
        }

        in_behind = in_front;
        in_front = in_front->next;
    }
}


void Front_Dequeue_Priority(struct Proccess_Type *Initial_Queue)
{
    if (Initial_Queue->front == NULL || Initial_Queue->front->next == NULL)
    {
        printf("No process left in da Q");
        return;
    }

    struct Nodes *curr = Initial_Queue->front;

    char *fname = Initial_Queue->front->fname;
    pid_t proc_pid = Initial_Queue->front->pid;
    int priority = Initial_Queue->front->priority;
    double exec = Initial_Queue->front->t_exec;
    double wait = Initial_Queue->front->t_wait;
    double waiting_process = Initial_Queue->front->waiting_process;
    double turn_around_time = Initial_Queue->front->turn_around_time;

    if (priority < 4)
    {
        priority++;
    }

    End_Enqueue_Priority(Initial_Queue, proc_pid, fname, priority, exec, wait, waiting_process,turn_around_time);

    Initial_Queue->front = Initial_Queue->front->next;
    free(curr);
}


/////////////////////////////PRINTING DETAILS 


void Proc_Details(struct Proccess_Type *Proccess_Type)
{
    struct Nodes *curr = Proccess_Type->front;
    int i = 1;
    printf("\nCurrently the queue : ");
    printf("\nNumber  PID  File Name  Priority  t_exec  t_wait \n");
    while (curr != NULL)
    {
        printf("(%d). %d, %s, %d, %f, %f\n", i, curr->pid, curr->fname, curr->priority, curr->t_exec, curr->t_wait - curr->turn_around_time);
        curr = curr->next;
        i++;
    }
    printf("\n");
}

void Scheduler_History(struct Proccess_Type* High_P_Queue)
{
    struct Nodes *curr = High_P_Queue->front;
    int i = 1;
    printf("History: ");
    printf("\n");
    printf("Number:  PID  FNAME  Priority  t_exec  t_wait\n");
    while (curr != NULL)
    {
       printf("%d  %d   %s         %d           %.3f           %d\n",
        i, curr->pid, curr->fname, curr->priority, curr->t_exec, (int)(curr->t_wait - curr->turn_around_time));

        i++;
        curr = curr->next;
    }
    
}
/////////////////////////////////////////////////////////////ALGORITHM FOR ROUND ROBIN 



void RR_Scheduler()
{
    double init_time = 0.0;
    double addition_time = 0.0;


    printf("Scheduler \n");
   
    while (Initial_Queue->front != NULL)
    {
        int index_start = 0;
        New_Queue = Init_Queue();
        struct Nodes *start_node = Initial_Queue->front;

        while (index_start < NCPU && start_node != NULL)
        {
            if (kill(start_node->pid, SIGCONT) == 0)
            {
                printf("Child process (PID: %d) has been resumed.\n", start_node->pid);
                usleep(5000000);
            }
            else
            {
                perror("kill");
            }
            index_start++;
            start_node = start_node->next;
        }

        usleep(TSLICE);
        init_time = addition_time;
        addition_time += (TSLICE / 1000.0);


        int index = 0;


        while (index < index_start && Initial_Queue->front != NULL)
        {
            struct Nodes *temp = Initial_Queue->front;
            int status;
            if (waitpid(temp->pid, &status, WNOHANG) != 0)
            {
                printf("process with PID-----> %d has been stopped.\n", temp->pid);
                temp->t_exec += (TSLICE / 1000.0);
                temp->waiting_process = addition_time;
                temp->t_wait += (init_time - temp->waiting_process);
                Enqueue_Proc(High_P_Queue, temp->pid, temp->fname, temp->priority, temp->t_exec, temp->t_wait, temp->waiting_process,temp->turn_around_time);
                End_process(Initial_Queue);
            }
            else
            {
                if (kill(temp->pid, SIGSTOP) == 0)
                {
                    printf("process with PID-----> %d has been stopped.\n", temp->pid);
                    temp->t_exec += (TSLICE / 1000.0);
                    temp->waiting_process = addition_time;
                    temp->t_wait += (init_time - temp->waiting_process);
                }
                else
                {
                    perror("kill");
                    printf("ERROR IN STOPPING");
                }
               
                if (temp->priority < 4){temp->priority += 1;}
                Enqueue_Proc(New_Queue, temp->pid, temp->fname, temp->priority, temp->t_exec, temp->t_wait, temp->waiting_process,temp->turn_around_time);
                End_process(Initial_Queue);
            }

        
            index++;
        }

        while (Ready_Queue->front != NULL)
        {
            printf("Adding it to the ready queue \n");
            pid_t pid = Ready_Queue->front->pid;
            char *fname = Ready_Queue->front->fname;
            int priority = Ready_Queue->front->priority;
            double exec_time = Ready_Queue->front->t_exec;
            double waiting_process = Ready_Queue->front->waiting_process;
            double turn_around_time = addition_time;
            double wait_time = Ready_Queue->front->t_wait;
            Front_Enqueue_Priority(Initial_Queue, pid, fname, priority, exec_time, wait_time, waiting_process,turn_around_time);
            End_process(Ready_Queue);
            printf("Sucessfully added to the ready Q...\n");

        
        }
        while(New_Queue->front != NULL){
            struct Nodes *temp = New_Queue->front;
            End_Enqueue_Priority(Initial_Queue, temp->pid, temp->fname, temp->priority, temp->t_exec, temp->t_wait, temp->waiting_process,temp->turn_around_time);
            End_process(New_Queue);
        }

    }

    printf("\nNO PROCESSESS REMAINING\n");
}

void *Scheduler(void *value)
{
    struct Proccess_Type *Initial_Queue = (struct Proccess_Type *)value;

    while (true)
    {
       
        if (Semaphore_Signal)
        {
            if (Initial_Queue->front != NULL)
            {
                sem_wait(&SHM->mutex);
                RR_Scheduler();
                sem_post(&SHM->mutex);
            }
            else
            {
                printf("No process left...\n");
            }
            Semaphore_Signal = 0;
        }
        
        sleep(5);
    }
    printf("Conducted Sucessfully...\n");
    return NULL;
}
////////////////////////////////////scheduler and threading to stop racing 




int main(int argc, char *argv[])
{
    struct sigaction custom_signal;
    memset(&custom_signal, 0, sizeof(custom_signal));
    custom_signal.sa_handler = Signal_Catching;
    sigaction(SIGUSR1, &custom_signal, NULL);
    sigaction(SIGUSR2, &custom_signal, NULL);
    signal(SIGINT,SIGINT_CATCH);

    Initial_Queue = Init_Queue();
    Ready_Queue = Init_Queue();
    High_P_Queue = Init_Queue();

    // Create a shared memory segment
    Shared_Memory_Segment = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
    if (Shared_Memory_Segment == -1) {
        perror("shm_open failed");
        return 1;
    }

    // Set the size of the shared memory segment
    ftruncate(Shared_Memory_Segment, sizeof(Sharedd_Memory));

    // Map the shared memory segment into your process's memory space
    SHM = (Sharedd_Memory*) mmap (0, sizeof(Sharedd_Memory), PROT_READ | PROT_WRITE, MAP_SHARED, Shared_Memory_Segment, 0);
    if (SHM == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }


    sem_init(&SHM->mutex, 1, 1);

    if (sem_init(&SHM->mutex, 1, 1) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    memcpy(&SHM->Initial_Queue, Initial_Queue, sizeof(struct Proccess_Type));
    memcpy(&SHM->High_P_Queue, High_P_Queue, sizeof(struct Proccess_Type));
    memcpy(&SHM->Ready_Queue, Ready_Queue, sizeof(struct Proccess_Type));
    memcpy(&SHM->New_Queue, Ready_Queue,sizeof(struct Proccess_Type));

    
    printf("Shell Has Been Started\n");
   
    bool running = true;

    NCPU = atoi(argv[1]);
    TSLICE = atof(argv[2]);
    TSLICE *= 1000.0;

    pthread_t threading;
    int sending_background = pthread_create(&threading, NULL, Scheduler, &Initial_Queue);

    if (sending_background)
    {
        printf("Error creating thread");
        return 1;
    }
    printf("\nScheduler Has Been Started\n\n");

    while (running)
    {
    
        printf(">>>>>>>> ");
        size_t size = 1024;
        char arr_fgets[4096];
        fgets(arr_fgets, 4096, stdin);
        char *NULL_ENDER = " \n";
        char *tokenizer;
        char **inp = (char **)malloc(4096 * sizeof(char *));
        tokenizer = strtok(arr_fgets, NULL_ENDER);
        int i = 0;
        inp[i] = tokenizer;
        while (tokenizer != NULL)
        {
            tokenizer = strtok(NULL, NULL_ENDER);
            i++;
            inp[i] = tokenizer;
        }
        int given_commands = 0;
        while (inp[given_commands] != NULL)
        {
            given_commands++;
        }
        if (given_commands == 0)
        {
            continue;
        }
        if (strcmp(inp[0], "submit") == 0) {
            pid_t new_process = fork();

            if (new_process == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } 
            else if (new_process == 0) {
                execlp(inp[1], inp[1], NULL);
                perror("execlp");
            } 
            else {
                int priority = (given_commands == 2) ? 1 : atoi(inp[2]);
                if (Semaphore_Signal == 0) {
                    Front_Enqueue_Priority(Initial_Queue, new_process, inp[1], priority, 0.0, 0.0, 0.0, 0.0);
                } 
                else {
                    Enqueue_Proc(Ready_Queue, new_process, inp[1], priority, 0.0, 0.0, 0.0, 0.0);
                }

                if (kill(new_process, SIGSTOP) == 0) {
                    printf("Process (PID: %d) added at the end of Queue\n", new_process);
                } 
                else {
                    perror("kill");
                }
            }
        } 
        else if (strcmp(inp[0], "make") == 0) {
            if (pthread_kill(threading, SIGUSR1) != 0) {
                perror("Signal sending failed");
                exit(1);
            }
        } 
        else if (strcmp(inp[0], "quit") == 0) {
            if (kill(getpid(), SIGUSR2) == 0) {
                printf("Signal sent to PID %d.\n", getpid());
            } 
            else {
                perror("kill");
            }
        } 
        else if (strcmp(inp[0], "Q") == 0) {
            Proc_Details(Initial_Queue);
        } 
        else if (strcmp(inp[0], "history") == 0) {
            Scheduler_History(High_P_Queue);
        } 
        else {
            printf("Wrong Input\n");
        }

    }
}
