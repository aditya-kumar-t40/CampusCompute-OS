/*
 * campus_compute.h
 * CampusCompute: Intelligent Job Scheduling and Resource Management
 * Operating Systems (UE25CS242A) - Level 3 Mini-Project
 */

#ifndef CAMPUS_COMPUTE_H
#define CAMPUS_COMPUTE_H

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

/* Constants */
#define MAX_JOBS 32
#define DEFAULT_TIME_QUANTUM 2
#define LOG_QUEUE_CAPACITY 5
#define LOG_MSG_LEN 256
#define LOG_FILE_PATH "campus_compute.log"
#define SHM_NAME "/cc_lab_shm_242a"
#define SEM_NAME "/cc_lab_sem_242a"

#define NUM_PROCESSES 5
#define NUM_RESOURCES 3

/* Job lifecycle status (FR1) */
typedef enum {
    JOB_READY = 0,
    JOB_RUNNING,
    JOB_WAITING,
    JOB_COMPLETED,
    JOB_FAILED
} JobStatus;

/* Job structure (FR1) */
typedef struct {
    int job_id;
    char job_name[32];
    int arrival_time;
    int cpu_burst;
    int remaining_time;
    int priority;          /* Lower numerical value = Higher priority */
    int start_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    JobStatus status;
} Job;

/* Scheduling summary metrics for policy comparison (FR2) */
typedef struct {
    char policy_name[32];
    double avg_turnaround_time;
    double avg_waiting_time;
    int total_completion_time;
} ScheduleSummary;

/* Bounded Producer-Consumer Log Queue (FR3) */
typedef struct {
    char entries[LOG_QUEUE_CAPACITY][LOG_MSG_LEN];
    int head;
    int tail;
    int count;
    int shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} BoundedLogQueue;

/* Shared Memory payload for IPC (FR4) */
typedef struct {
    pid_t producer_pid;
    pid_t consumer_pid;
    int active_jobs_completed;
    int total_cpu_cycles;
    char status_report[128];
    int sync_flag;
} SharedClusterState;

/* Function Prototypes - FR1 & FR2: Job Management & CPU Scheduling (scheduler.c) */
const char* job_status_str(JobStatus status);
int validate_jobs(const Job *jobs, int n);
int load_default_jobs(Job *jobs);
void print_job_table(const Job *jobs, int n, const char *title);
ScheduleSummary run_fcfs(const Job *input_jobs, int n, int verbose);
ScheduleSummary run_round_robin(const Job *input_jobs, int n, int quantum, int verbose);
ScheduleSummary run_priority_scheduling(const Job *input_jobs, int n, int verbose);
int run_scheduling_comparison(const Job *input_jobs, int n, int quantum);

/* Function Prototypes - FR3: Concurrent Activity Logger (logger.c) */
int run_logger_demo(void);
int append_direct_log(const char *tag, const char *message);

/* Function Prototypes - FR5: Banker's Resource Safety Analysis (banker.c) */
int run_banker_demo(int test_safe_state);

/* Function Prototypes - FR4, FR6, FR7: IPC, Worker Monitoring & Resource Limits (ipc_worker.c) */
int run_shm_ipc_demo(int keep_resources_for_inspect);
int run_sem_cleanup_demo(void);
int run_worker_demo(int simulate_failure);
int run_rlimit_demo(void);
int cleanup_all_resources(int verbose);

#endif /* CAMPUS_COMPUTE_H */
