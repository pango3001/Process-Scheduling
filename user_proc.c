#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define TRUE 1
#define FALSE 0

FILE* logFile;//log file
const key_t PCB_TABLE_KEY = 110594;//key for shared PCB Table
const key_t CLOCK_KEY = 110197;//key for shared simulated clock 
const key_t MSG_KEY = 052455;//key for message queue
int pcbTableId;//shmid for PCB Table
int clockId;//shmid for simulated clock
int msqid;//id for message queue

//simulated time value
//used for the simulated clock
typedef struct {
    unsigned int s;
    unsigned int ns;
} simtime_t;

//pseudo-process control block
//used for PCB Table
typedef struct {
    //simulated process id, range is [0,18]
    int pid;
    //Process priority
    int priority;
    //if the process is ready to run
    int isReady;
    //Arrivial time
    simtime_t arrivalTime;
    //CPU time used
    simtime_t cpuTime;
    //Time in the system
    simtime_t sysTime;
    //Time used in the last burst
    simtime_t burstTime;
    //Total sleep time. time waiting for an event
    simtime_t waitTime;


} pcb_t;

//msg struct for msgqueue
typedef struct {
    long mtype;
    int mvalue;
} mymsg_t;

//increment given simulated time by given increment
void increment_sim_time(simtime_t* simTime, int increment) {
    simTime->ns += increment;
    if (simTime->ns >= 1000000000) {
        simTime->ns -= 1000000000;
        simTime->s += 1;
    }
}
// returns a - b
simtime_t subtract_sim_times(simtime_t a, simtime_t b) {
    simtime_t diff = { .s = a.s - b.s,
                      .ns = a.ns - b.ns };
    if (diff.ns < 0) {
        diff.ns += 1000000000;
        diff.s -= 1;
    }
    return diff;
}
//returns a + b
simtime_t add_sim_times(simtime_t a, simtime_t b) {
    simtime_t sum = { .s = a.s + b.s,
                      .ns = a.ns + b.ns };
    if (sum.ns >= 1000000000) {
        sum.ns -= 1000000000;
        sum.s += 1;
    }
    return sum;
}

//returns simtime / divisor
simtime_t divide_sim_time(simtime_t simTime, int divisor) {
    simtime_t quotient = { .s = simTime.s / divisor, .ns = simTime.ns / divisor };
    return quotient;
}

pcb_t create_pcb(int priority, int pid, simtime_t currentTime) {
    pcb_t pcb = { .pid = pid,
                  .priority = priority,
                  .isReady = TRUE,
                  .arrivalTime = {.s = currentTime.s, .ns = currentTime.ns},
                  .cpuTime = {.s = 0, .ns = 0},
                  .sysTime = {.s = 0, .ns = 0},
                  .burstTime = {.s = 0, .ns = 0},
                  .waitTime = {.s = 0, .ns = 0} };
    return pcb;
}

pcb_t* attach_pcb_table();
simtime_t* attach_sim_clock();
void get_clock_and_table(int n);
int get_outcome();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./user pid msqid quantum\n");
        exit(EXIT_SUCCESS);
    }
    int pid;
    mymsg_t msg;
    int quantum;
    int outcome = 0;
    pid = atoi(argv[1]);
    msqid = atoi(argv[2]);
    quantum = atoi(argv[3]);
    srand(time(0) + (pid + 1));  // seeding rand. seeding w/ time(0) caused
                                 // processes spawned too close to have same seed
    pcb_t* table;
    simtime_t* simClock;
    simtime_t timeBlocked;//holds the time that the process was blocked at
    simtime_t event;  // time of the event that will unblock the process
    int burst;//burst to calculate unblock time
    get_clock_and_table(pid);
    table = attach_pcb_table();
    simClock = attach_sim_clock();

    // while loop to wait for messages from oss until we terminate.
    while (outcome != 1) {  // outcome == 1 means terminate
        if ((msgrcv(msqid, &msg, sizeof(msg.mvalue), (pid + 1), 0)) == -1) {
            perror("./user: Error: msgrcv ");
            exit(EXIT_FAILURE);
        }
        /* message response based on if terminating, blocked or neither
         * oss will know process is blocked if mvalue < 0
         *                       is terminating if 0 <= mvalue < 100
         *                       is using full quantum if mvalue == 100
         * */
        outcome = get_outcome();
        switch (outcome) {
        case 0:  // full
            msg.mvalue = 100;
            break;
        case 1:  // term
            msg.mvalue = (rand() % 99) + 1;
            break;
        case 2:  // block
            msg.mvalue = ((rand() % 99) + 1) * -1;
            timeBlocked.s = simClock->s;
            timeBlocked.ns = simClock->ns;
            burst = msg.mvalue * (quantum / 100) * pow(2.0, (double)table[pid].priority);
            event.s = (rand() % 4) + 1;//generating r [0,5]
            event.ns = (rand() % 1000) * 1000000; //generating s [0, 999]. * 1000000 to convert to ns
            // add to wait time total
            table[pid].waitTime = add_sim_times(table[pid].waitTime, event);
            event = add_sim_times(event, timeBlocked);//event time = current time + r.s
            increment_sim_time(&event, (burst * -1));
            // set status to blocked before telling oss to avoid race
            // condition. OSS is waiting for a message response so it
            // cant possibly check our isReady variable yet
            table[pid].isReady = FALSE;
            break;
        default:
            break;
        }                       // end switch
        msg.mtype = pid + 100;  // oss is waiting for a msg w/ type pid+100
        //printf("USER: sending type: %ld from pid: %d", msg.mtype, pid);
        if (msgsnd(msqid, &msg, sizeof(msg.mvalue), 0) == -1) {
            perror("./user: Error: msgsnd ");
            exit(EXIT_FAILURE);
        }
        //printf(", sent type: %ld from pid: %d\n", msg.mtype, pid);
        // BLocked Outcome
        // already sent message to oss that we are blocked
        // set to
        if (outcome == 2) {
            // while loop to wait for event time to pass
            while (table[pid].isReady == FALSE) {
                //printf("waiting %ds%9dns\n", event.s, event.ns);
                if (event.s > simClock->s) {
                    table[pid].isReady = TRUE;
                }
                else if (event.ns >= simClock->ns && event.s >= simClock->s) {
                    table[pid].isReady = TRUE;
                }
            }
        }
    }  // end while. no longer sending or recieving messages
    // printf("%d term\n", pid);
    return 0;
}

pcb_t* attach_pcb_table() {
    pcb_t* pcbTable;
    pcbTable = shmat(pcbTableId, NULL, 0);
    if (pcbTableId < 0) {  // error
        perror("./user: Error: shmat ");
        exit(EXIT_FAILURE);
    }
    return pcbTable;
}

simtime_t* attach_sim_clock() {
    simtime_t* simClock;
    simClock = shmat(clockId, NULL, 0);
    if (clockId < 0) {  // error
        perror("./user: Error: shmat ");
        exit(EXIT_FAILURE);
    }
    return simClock;
}

void get_clock_and_table(int n) {
    // Getting shared memory for the simulated clock
    clockId = shmget(CLOCK_KEY, sizeof(simtime_t), IPC_CREAT | 0777);
    if (clockId < 0) {  // error
        perror("./user: Error: shmget ");
        exit(EXIT_FAILURE);
    }
    // Getting shared memory for the pcb table
    pcbTableId = shmget(PCB_TABLE_KEY, sizeof(pcb_t) * (n + 1), IPC_CREAT | 0777);
    if (pcbTableId < 0) {
        perror("./user: Error: shmget ");
        exit(EXIT_FAILURE);
    }
    return;
}

// 0: not terminating or blocked, 1: terminating, 2:not terminating but blocked
int get_outcome() {
    int tPercent = 5;   //% chance of terminating
    int bPercent = 5;  //% chance of getting blocked
    int terminating = ((rand() % 100) + 1) <= tPercent ? TRUE : FALSE;
    int blocked = ((rand() % 100) + 1) <= bPercent ? TRUE : FALSE;
    if (terminating)
        return 1;
    if (blocked)
        return 2;
    // not blocked or terminating
    return 0;
}