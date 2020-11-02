// Jesse McCarville-Schueths
// Oct 25, 2020
// Assignment 4
// Scheduled Processes


#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_PCB 18 // max amount of processes in the system at once

// message queue
typedef struct {
    long mess_ID;
    int mess_quant;
} message;

// for simulated clock
typedef struct {
    unsigned int simu_seconds;
    unsigned int simu_nanosecs;
} simu_time;

// PCB Table
typedef struct {
    unsigned int pid;  // max is 18
    int priority;
    int onDeck;
    simu_time arrivalTime;
    simu_time cpuTime;
    simu_time sysTime; //Time in the system
    simu_time burstTime;  //Time used in the last burst
    simu_time waitTime;  //Total sleep time. time waiting for an event
} process_table;

// Queue
typedef struct {
    unsigned int head;
    unsigned int tail;
    unsigned int size;
    unsigned int items;
    int* data;
} queue_t;



FILE* logFile;//log file
const key_t PCB_TABLE_KEY = 110594;//key for shared PCB Table
const key_t CLOCK_KEY = 110197;//key for shared simulated clock 
const key_t MSG_KEY = 052455;//key for message queue
int pcbTableId;//shmid for PCB Table
int clockId;//shmid for simulated clock
int messageID;//id for message queue

// Prototypes

void create_msqueue();
FILE* open_file(char*, char*, char*);
static void time_out();
void cleanup();
void remove_shm();
process_table* create_table(int);
int get_sim_pid(int*, int);
int rand_priority(int);
int check_blocked(int*, process_table*, int);
void oss(int);
int dispatch(int, int, int, simu_time, int, int*);
simu_time* create_sim_clock();
simu_time get_next_process_time(simu_time, simu_time);
int should_spawn(int, simu_time, simu_time, int, int);
void increment_sim_time(simu_time* simTime, int increment);
queue_t* create_queue(int size);
simu_time subtract_sim_times(simu_time a, simu_time b);
simu_time add_sim_times(simu_time a, simu_time b);
simu_time divide_sim_time(simu_time simTime, int divisor);
process_table create_pcb(int priority, int pid, simu_time currentTime);
void enqueue(queue_t* queue, int pid);
int dequeue(queue_t* queue);



int main(int argc, char* argv[]) {
    system("clear");  // clear screen
    char* log = "oss.log";  // log file
    // Open log file
    logFile = open_file(log, "w", "./oss: Error: ");
    // Terminate after 3s
    signal(SIGALRM, time_out);
    alarm(20);
    srand(time(0));// seed rand
    printf("Running sim...\n");
    oss(MAX_PCB);
    printf("Ending sim and cleaned up!\n");
    printf("Please use the command 'cat oss.log' to view log. \n");

    // safe cleanup
    msgctl(messageID, IPC_RMID, NULL);  //delete msgqueue
    remove_shm();
    fclose(logFile);  // close log

    return 0;
}

/*fopen with simple error check*/
FILE* open_file(char* fname, char* opts, char* error) {
    FILE* fp = fopen(fname, opts);
    if (fp == NULL) {  // error opening file
        perror(error);
        cleanup();
    }
    return fp;
}
/*SIGALRM handler*/
static void time_out() {
    fprintf(stderr, "3 second timeout\n");
    fprintf(stderr, "Killing user/child processes\n");
    cleanup();
    exit(EXIT_SUCCESS);
}
/*delete shared memory, terminate children*/
void cleanup() {
    fclose(logFile);
    remove_shm();                   // remove shared memory
    msgctl(messageID, IPC_RMID, NULL);  // delete msgqueue
    kill(0, SIGTERM);               // terminate users/children
    return;
}
/*Remove the simulated clock and pcb table from shared memory*/
void remove_shm() {
    shmctl(clockId, IPC_RMID, NULL);
    shmctl(pcbTableId, IPC_RMID, NULL);
    return;
}
/*Create pcb table in shared memory for n processes*/
process_table* create_table(int n) {
    process_table* table;
    pcbTableId = shmget(PCB_TABLE_KEY, sizeof(process_table) * n, IPC_CREAT | 0777);
    if (pcbTableId < 0) {  // error
        perror("./oss: Error: shmget ");
        cleanup();
    }
    table = shmat(pcbTableId, NULL, 0);
    if (table < 0) {  // error
        perror("./oss: Error: shmat ");
        cleanup();
    }
    return table;
}
/*Create a simulated clock in shared memory initialized to 0s0ns*/
simu_time* create_sim_clock() {
    simu_time* simClock;
    clockId = shmget(CLOCK_KEY, sizeof(simu_time), IPC_CREAT | 0777);
    if (clockId < 0) {  // error
        perror("./oss: Error: shmget ");
        cleanup();
    }
    simClock = shmat(clockId, NULL, 0);
    if (simClock < 0) {  // error
        perror("./user: Error: shmat ");
        cleanup();
    }
    simClock->simu_seconds= 0;
    simClock->simu_nanosecs = 0;
    return simClock;
}
/*Create a message queue*/
void create_msqueue() {
    messageID = msgget(MSG_KEY, 0666 | IPC_CREAT);
    if (messageID < 0) {
        perror("./oss: Error: msgget ");
        cleanup();
    }
    return;
}
/*Return a pid if there is one available, returns -1 otherwise*/
int get_sim_pid(int* pids, int pidsCount) {
    int i;
    for (i = 0; i < pidsCount; i++) {
        if (pids[i]) {  // sim pid i is available
            return i;     // return available pid
        }
    }
    return -1;  // no available pids
}
/*return a random simtime in range [0, max] + current time*/
simu_time get_next_process_time(simu_time max, simu_time currentTime) {
    simu_time nextTime = { .simu_nanosecs = (rand() % (max.simu_nanosecs + 1)) + currentTime.simu_nanosecs,
                          .simu_seconds= (rand() % (max.simu_seconds+ 1)) + currentTime.simu_seconds};
    if (nextTime.simu_nanosecs >= 1000000000) {
        nextTime.simu_seconds+= 1;
        nextTime.simu_nanosecs -= 1000000000;
    }
    return nextTime;
}
/* Returns 0 chance % of the time on average. returns 1 otherwise*/
int rand_priority(int chance) {
    if ((rand() % 101) <= chance)
        return 0;
    else
        return 1;
}

int should_spawn(int pid, simu_time next, simu_time now, int generated,
    int max) {
    if (generated >= max)  // generated enough/too many processes
        return 0;
    if (pid < 0)  // no available pids
        return 0;
    // not time for next process
    if (next.simu_seconds> now.simu_seconds)
        return 0;
    // more specific not time for next process
    if (next.simu_seconds>= now.simu_seconds&& next.simu_nanosecs > now.simu_nanosecs)
        return 0;

    return 1; // return true (should spawn new process)
}

int check_blocked(int* blocked, process_table* table, int count) {
    int i; //loop iterator
    for (i = 0; i < count; i++) {
        if (blocked[i] == 1) {
            if (table[i].onDeck == 1) {
                return i;
            }
        }
    }
    return -1;
}
/*Dispatches a process by sending a message witht he appropriate time quantum to
 * the given pid
 * waits to recieve a response and returns that response */
int dispatch(int pid, int priority, int messageID, simu_time currentTime, int quantum, int* lines) {
    message msg;                                     // message to be sent
    quantum = quantum * pow(2.0, (double)priority);  // i = queue #, slice = 2^i * quantum
    msg.mess_ID = pid + 1;                             // pids recieve messages of type (their pid) + 1
    msg.mess_quant = quantum;
    fprintf(logFile, "OSS: Dispatching process with PID: %3d Queue: %d TIME: %ds%09dns\n", pid, priority, currentTime.simu_seconds, currentTime.simu_nanosecs);
    *lines += 1;
    // send the message
    if (msgsnd(messageID, &msg, sizeof(msg.mess_quant), 0) == -1) {
        perror("./oss: Error: msgsnd ");
        cleanup();
    }
    // immediately wait for the response
    if ((msgrcv(messageID, &msg, sizeof(msg.mess_quant), pid + 100, 0)) == -1) {
        perror("./oss: Error: msgrcv ");
        cleanup();
    }
    return msg.mess_quant;
}
/**OSS: Operating System Simulator**/
void oss(int maxProcesses) {
    /*Scheduling structures*/
    process_table* table;// Process control block table
    simu_time* simClock;// simulated system clock
    queue_t* rrQueue; // round robin queue
    queue_t* queue1; // highest level of mlfq
    queue_t* queue2; // mid level
    queue_t* queue3;  // lowest level
    int blockedPids[maxProcesses];    //blocked "queue"
    int availablePids[maxProcesses];  // pseudo bit vector of available pids

    int schedInc = 1000;// scheduling overhead
    int idleInc = 100000;// idle increment
    int blockedInc = 1000000; // blocked queue check increment
    int quantum = 10000000; // base time quantum
    int burst; // actual time used by the process
    int response; // response from process. will be percentage of quantum used

    simu_time maxTimeBetweenNewProcesses = { .simu_seconds= 0, .simu_nanosecs = 500000000 };
    simu_time nextProcess; // holds the time we should spawn the next process
    int terminated = 0; // counter of terminated processes
    int generated = 0; // counter of generated processes
    int lines = 0;  // lines written to the file
    int maxTotalProcesses = 100; // how many processes to generate total
    int simPid; // holds a simulated pid
    int priority;  // holds priority of a process
    char simPidArg[3]; // exec arg 2. simulated pid as a string
    char messageIDArg[10];// exec arg 3. messageID as string
    char quantumArg[10];
    sprintf(quantumArg, "%d", quantum);

    int i = 0;  // loop iterator
    int pid;    // holds wait() and fork() return values
	
    /*Statistics*/
    simu_time totalCPU = { .simu_seconds= 0, .simu_nanosecs = 0 };
    simu_time totalSYS = { .simu_seconds= 0, .simu_nanosecs = 0 };
    simu_time totalIdle = { .simu_seconds= 0, .simu_nanosecs = 0 };
    simu_time totalWait = { .simu_seconds= 0, .simu_nanosecs = 0 };
    double avgCPU = 0.0;
    double avgSYS = 0.0;
    double avgWait = 0.0;

    /*Setup*/
    table = create_table(maxProcesses);         // setup shared pcb table
    simClock = create_sim_clock(maxProcesses);  // setup shared simulated clock
    rrQueue = create_queue(maxProcesses);       // setup round robin queue
    queue1 = create_queue(maxProcesses);        // setup MLFQ. highest level/priority
    queue2 = create_queue(maxProcesses);        // mid level/priority
    queue3 = create_queue(maxProcesses);        // lowest level/priority
    create_msqueue();                           // set up message queue.sets global messageID variable
    sprintf(messageIDArg, "%d", messageID);             // write messageID to messageID string arg
    // Loop through available pids to set all to 1(available)
    for (i = 0; i < maxProcesses; i++) {
        blockedPids[i] = 0;//set blocked "queue" to empty
        availablePids[i] = 1;
    }
    // get time to spawn next process
    nextProcess = get_next_process_time(maxTimeBetweenNewProcesses, (*simClock));

    while (terminated < maxTotalProcesses) {
        // if we have written 9000 lines stop generating
        // criteria is to stop at 10000 however lines will continue to be written
        // because there are still processes in the system
        if (lines >= 9000)
            maxTotalProcesses = generated;

        // check for an available pid
        simPid = get_sim_pid(availablePids, maxProcesses);

        // if less than 100 processes create a new one
        if (should_spawn(simPid, nextProcess, (*simClock), generated, maxTotalProcesses)) {
            sprintf(simPidArg, "%d", simPid);  // write simpid to simpid string arg
            availablePids[simPid] = 0;     // set pid to unavailable
            // get random priority(0:real time or 1:user)
            priority = rand_priority(5);
            fprintf(logFile, "OSS: Generating process PID %d in queue %d at %ds%09d nanoseconds\n",
                simPid, priority, simClock->simu_seconds, simClock->simu_nanosecs);
            // create pcb for new process at available pid
            table[simPid] = create_pcb(priority, simPid, (*simClock));
            // queue in round robin if real-time(priority == 0)
            if (priority == 0)
                enqueue(rrQueue, simPid);
            else  // queue in MLFQ otherwise
                enqueue(queue1, simPid);
            // fork a new process
            pid = fork();
            if (pid < 0) {  // error
                perror("./oss: Error: fork ");
                cleanup();
            }
            else if (pid == 0) {  // child
                execl("./user_proc", "user_proc", simPidArg, messageIDArg, quantumArg, (char*)NULL);
            }
            // parent
            generated += 1;  // increment generated processes counter
            nextProcess = get_next_process_time(maxTimeBetweenNewProcesses, (*simClock));
            increment_sim_time(simClock, 10000);
        }
        /* Check blocked queue */
        else if ((simPid = check_blocked(blockedPids, table, maxProcesses)) >= 0) {
            blockedPids[simPid] = 0;//remove from blocked "queue"
            fprintf(logFile, "OSS: Unblocked. PID: %3d TIME: %ds%09dns\n", simPid, simClock->simu_seconds, simClock->simu_nanosecs);
            if (table[simPid].priority == 0) {
                fprintf(logFile, "OSS: PID: %3d -> Round Robin\n", simPid);
                enqueue(rrQueue, simPid);
            }
            else {
                fprintf(logFile, "OSS: PID: %3d -> Queue 1\n", simPid);
                enqueue(queue1, simPid);
            }
            increment_sim_time(simClock, blockedInc);  // and blocked queue overhead to the clock
        }
        // Round Robin
        else if (rrQueue->items > 0) {
            increment_sim_time(simClock, schedInc);  // increment for scheduling overhead
            simPid = dequeue(rrQueue);               // get pid at head of the queue
            priority = table[simPid].priority;       // get stored priority
            // dispatch the process
            response = dispatch(simPid, priority, messageID, (*simClock), quantum, &lines);
            // calculate burst time
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100) {                  // Used full time slice
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS: PID: %3d -> Round Robin\n", simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                enqueue(rrQueue, simPid);
            }
            else if (response < 0) {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS: PID: %3d -> Blocked queue\n", simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                blockedPids[simPid] = 1;//add to blocked "queue"
            }
            else {                                // Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);                     // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = 1;  // set simpid to available
                terminated += 1;               // increment terminated process counter
                fprintf(logFile, "OSS: Terminated PID: %3d Used: %9dns\n", simPid, burst);
            }
        }
        /*MLFQ
         * Queue 1*/
        else if (queue1->items > 0) {
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue1);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, messageID, (*simClock), quantum, &lines);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100) {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "OSS: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS: PID: %3d -> Queue 2\n", simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 2;
                enqueue(queue2, simPid);
            }
            else if (response < 0) {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS: PID: %3d -> Blocked queue\n", simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = 1;//add to blocked "queue"
            }
            else {                                // Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);                     // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = 1;  // set simpid to available
                terminated += 1;               // increment terminated processes counter
                fprintf(logFile, "OSS: Terminated PID: %3d Used: %9dns\n", simPid, burst);
            }  // end response ifs
        }    // end queue 1 check
        /* Queue 2 */
        else if (queue2->items > 0) {
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue2);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, messageID, (*simClock), quantum, &lines);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100) {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "OSS: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS: PID: %3d -> Queue 3\n", simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 3;
                enqueue(queue3, simPid);
            }
            else if (response < 0) {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS: PID: %3d -> Blocked queue\n", simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = 1;//add to blocked "queue" 
            }
            else {                                // Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);                     // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = 1;  // set simpid to available
                terminated += 1;               // increment terminated processes counter
                fprintf(logFile, "OSS: Terminated PID: %3d Used: %9dns\n", simPid, burst);
            }  // end response ifs
        }    // end queue 2 check
        // 3rd Queue
        else if (queue3->items > 0) {
            increment_sim_time(simClock, schedInc);
            simPid = dequeue(queue3);
            priority = table[simPid].priority;
            response = dispatch(simPid, priority, messageID, (*simClock), quantum, &lines);
            burst = response * (quantum / 100) * pow(2.0, (double)priority);
            if (response == 100) {
                increment_sim_time(simClock, burst);
                fprintf(logFile, "OSS: Full Slice PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS: PID: %3d -> Queue 3\n", simPid);
                // updat pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                enqueue(queue3, simPid);
            }
            else if (response < 0) {  // BLocked
                burst = burst * -1;
                increment_sim_time(simClock, burst);  // increment the clock
                fprintf(logFile, "OSS: Blocked    PID: %3d Used: %9dns\n", simPid, burst);
                fprintf(logFile, "OSS: PID: %3d -> Blocked queue\n", simPid);
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                table[simPid].priority = 1;  // set priority back to highest level
                blockedPids[simPid] = 1;//add to blocked "queue"
            }
            else {                                // Terminated
                increment_sim_time(simClock, burst);  // increment the clock
                pid = wait(NULL);                     // wait for child to finish
                // update pcb
                increment_sim_time(&table[simPid].cpuTime, burst);
                table[simPid].sysTime = subtract_sim_times((*simClock), table[simPid].arrivalTime);
                // update totals
                totalCPU = add_sim_times(totalCPU, table[simPid].cpuTime);
                totalSYS = add_sim_times(totalSYS, table[simPid].sysTime);
                totalWait = add_sim_times(totalWait, table[simPid].waitTime);
                availablePids[simPid] = 1;  // set simpid to available
                terminated += 1;               // increment terminated processes counter
                fprintf(logFile, "OSS: Terminated PID: %3d Used: %9dns\n", simPid, burst);
            }  // end response ifs
        }    // end queue 3 check
        /* IDLE: empty queues and nothing ready to spawn */
        else {
            increment_sim_time(&totalIdle, idleInc);
            increment_sim_time(simClock, idleInc);
        }
    }
    // easier to divide decimals than simtime_t
    printf("Total Run Time:  %d.%ds\n", simClock->simu_seconds, simClock->simu_nanosecs / 10000000);
    avgCPU = (totalCPU.simu_seconds+ (0.000000001 * totalCPU.simu_nanosecs)) / ((double)generated);
    avgSYS = (totalSYS.simu_seconds+ (0.000000001 * totalSYS.simu_nanosecs)) / ((double)generated);
    avgWait = (totalWait.simu_seconds+ (0.000000001 * totalWait.simu_nanosecs)) / ((double)generated);
    printf("\n\tStats about simulation...\n")
    printf("\tTotal Processes: %d\n", generated);
    printf("\tAvg. Turnaround: %.2fs\n", avgSYS);
    printf("\tAvg. CPU Time:   %.2fs\n", avgCPU);
    printf("\tAvg. Wait Time:  %.2fs\n", avgWait);
    printf("\tAvg. Sleep Time: %.2fs\n", (avgSYS - avgCPU));
    printf("\tTotal Idle Time: %d.%ds\n", totalIdle.simu_seconds, totalIdle.simu_nanosecs / 10000000);
    return;
}


//increment given simulated time by given increment
void increment_sim_time(simu_time* simTime, int increment) {
    simTime->simu_nanosecs += increment;
    if (simTime->simu_nanosecs >= 1000000000) {
        simTime->simu_nanosecs -= 1000000000;
        simTime->simu_seconds+= 1;
    }
}

queue_t* create_queue(int size) {
    queue_t* queue = (queue_t*)malloc(sizeof(queue_t));
    queue->head = 0;
    queue->tail = 0;
    queue->size = size;
    queue->data = (int*)malloc(size * sizeof(int));
    queue->items = 0;
    int i;
    for (i = 0; i < size; i++)
        queue->data[i] = -1;
    return queue;
}

void enqueue(queue_t* queue, int pid) {
    queue->data[queue->tail] = pid;//add pid to queue
    queue->tail = (queue->tail + 1) % queue->size;
    queue->items += 1;
    return;
}

int dequeue(queue_t* queue) {
    int pid = queue->data[queue->head];
    queue->head = (queue->head + 1) % queue->size;
    queue->items -= 1;
    return pid;
}


// returns a - b
simu_time subtract_sim_times(simu_time a, simu_time b) {
    simu_time diff = { .simu_seconds = a.simu_seconds - b.simu_seconds,
                      .simu_nanosecs = a.simu_nanosecs - b.simu_nanosecs };
    if (diff.simu_nanosecs < 0) {
        diff.simu_nanosecs += 1000000000;
        diff.simu_seconds -= 1;
    }
    return diff;
}

//returns a + b
simu_time add_sim_times(simu_time a, simu_time b) {
    simu_time sum = { .simu_seconds= a.simu_seconds + b.simu_seconds,
                      .simu_nanosecs = a.simu_nanosecs + b.simu_nanosecs };
    if (sum.simu_nanosecs >= 1000000000) {
        sum.simu_nanosecs -= 1000000000;
        sum.simu_seconds += 1;
    }
    return sum;
}

//returns simtime / divisor
simu_time divide_sim_time(simu_time simTime, int divisor) {
    simu_time quotient = { .simu_seconds = simTime.simu_seconds / divisor, .simu_nanosecs = simTime.simu_nanosecs / divisor };
    return quotient;
}

process_table create_pcb(int priority, int pid, simu_time currentTime) {
    process_table pcb = { .pid = pid,
                  .priority = priority,
                  .onDeck = 1,
                  .arrivalTime = {.simu_seconds = currentTime.simu_seconds, .simu_nanosecs = currentTime.simu_nanosecs},
                  .cpuTime = {.simu_seconds = 0, .simu_nanosecs = 0},
                  .sysTime = {.simu_seconds= 0, .simu_nanosecs = 0},
                  .burstTime = {.simu_seconds= 0, .simu_nanosecs = 0},
                  .waitTime = {.simu_seconds= 0, .simu_nanosecs = 0} };
    return pcb;
}