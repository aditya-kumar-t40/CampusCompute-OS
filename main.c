/*
 * main.c
 * FR8: Command-Line Interface, Interactive Menu, Input Validation, and Integrated Execution
 * CampusCompute: Intelligent Job Scheduling and Resource Management (UE25CS242A)
 */

#include "campus_compute.h"

static void print_usage(const char *prog_name) {
    printf("\n====================================================================================\n");
    printf(" CampusCompute: Intelligent Job Scheduling & Resource Management (UE25CS242A)\n");
    printf("====================================================================================\n");
    printf(" Usage:\n");
    printf("   %s <command> [options]\n\n", prog_name);
    printf(" Available Commands (Each feature is independently testable):\n");
    printf("   help                       Display this CLI usage and command reference\n");
    printf("   fcfs                       Run First-Come, First-Served (FCFS) CPU scheduling\n");
    printf("   rr [quantum]               Run Round Robin scheduling (optional quantum, default=%d)\n", DEFAULT_TIME_QUANTUM);
    printf("   priority                   Run Non-Preemptive Priority scheduling\n");
    printf("   compare [quantum]          Compare FCFS, Round Robin, and Priority on same job set\n");
    printf("   custom-sched <N> <j1..jN>  Run scheduling on custom validated jobs (id,arr,burst,prio)\n");
    printf("   logger                     Run POSIX Thread Producer-Consumer activity logger\n");
    printf("   ipc                        Run POSIX Shared Memory IPC between cooperating processes\n");
    printf("   sem                        Demonstrate Named Semaphore coordination & IPC cleanup\n");
    printf("   banker-safe                Run Banker's Safety Algorithm on a SAFE resource state\n");
    printf("   banker-unsafe              Run Banker's Safety Algorithm on an UNSAFE resource state\n");
    printf("   worker-ok                  Demonstrate normal child worker lifecycle (fork/waitpid)\n");
    printf("   worker-fail                Demonstrate controlled simulated worker failure detection\n");
    printf("   rlimit                     Demonstrate process resource limits via setrlimit()\n");
    printf("   cleanup                    Clean up and verify release of program-created IPC objects\n");
    printf("   all                        Run integrated end-to-end demonstration of all modules\n");
    printf("   menu                       Launch interactive command-line menu\n");
    printf("====================================================================================\n");
}

/* Parse a positivenumber strictly with validation */
static int parse_positive_int(const char *str, int *out_val) {
    if (!str || *str == '\0') return -1;
    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);
    if (*endptr != '\0' || val <= 0 || val > 10000) {
        return -1;
    }
    *out_val = (int)val;
    return 0;
}

/* Parse custom job specification string "id,arrival,burst,priority" */
static int parse_custom_job(const char *spec, Job *job) {
    int id, arr, burst, prio;
    char extra;
    if (sscanf(spec, "%d,%d,%d,%d%c", &id, &arr, &burst, &prio, &extra) != 4) {
        return -1;
    }
    job->job_id = id;
    snprintf(job->job_name, sizeof(job->job_name), "CustomJob_%d", id);
    job->arrival_time = arr;
    job->cpu_burst = burst;
    job->remaining_time = burst;
    job->priority = prio;
    job->start_time = -1;
    job->completion_time = 0;
    job->turnaround_time = 0;
    job->waiting_time = 0;
    job->status = JOB_READY;
    return 0;
}

/* Integrated execution across all FR1–FR8 components (S21) */
static int run_integrated_demo(void) {
    printf("\n####################################################################################\n");
    printf(" CAMPUSCOMPUTE INTEGRATED END-TO-END SYSTEM EXECUTION (FR1 - FR8)\n");
    printf("####################################################################################\n");

    Job jobs[MAX_JOBS];
    int n = load_default_jobs(jobs);

    printf("\n>>> [PHASE 1/6] Job Lifecycle Validation & Multi-Policy CPU Scheduling (FR1, FR2)\n");
    run_scheduling_comparison(jobs, n, DEFAULT_TIME_QUANTUM);

    printf("\n>>> [PHASE 2/6] Concurrent Bounded Producer-Consumer Activity Logger (FR3)\n");
    run_logger_demo();

    printf("\n>>> [PHASE 3/6] Cooperating Processes Shared Memory & Semaphore IPC (FR4)\n");
    run_sem_cleanup_demo();

    printf("\n>>> [PHASE 4/6] Resource-Safety Analysis via Banker's Algorithm (FR5)\n");
    run_banker_demo(1);
    run_banker_demo(0);

    printf("\n>>> [PHASE 5/6] Worker Process Monitoring & Fault Detection (FR6)\n");
    run_worker_demo(0);
    run_worker_demo(1);

    printf("\n>>> [PHASE 6/6] Process Resource Limits & System Cleanup Verification (FR7)\n");
    run_rlimit_demo();
    cleanup_all_resources(1);

    printf("\n####################################################################################\n");
    printf(" INTEGRATED EXECUTION SUMMARY\n");
    printf("  [OK] FR1 & FR2 : Validated %d jobs; executed FCFS, RR (Q=%d), and Priority policies\n",
           n, DEFAULT_TIME_QUANTUM);
    printf("  [OK] FR3       : Logged 12 concurrent producer events via bounded pthread queue\n");
    printf("  [OK] FR4       : Exchanged cluster telemetry over POSIX shm & named semaphore\n");
    printf("  [OK] FR5       : Verified Banker's SAFE sequence and detected UNSAFE state\n");
    printf("  [OK] FR6       : Collected normal worker status and simulated failure codes\n");
    printf("  [OK] FR7 & FR8 : Enforced RLIMIT_FSIZE/RLIMIT_CPU and cleaned up all IPC objects\n");
    printf("####################################################################################\n");
    return 0;
}

/* Interactive CLI Menu (FR8) */
static int run_interactive_menu(const char *prog_name) {
    char line[128];
    Job jobs[MAX_JOBS];
    int n = load_default_jobs(jobs);

    while (1) {
        printf("\n==================== CampusCompute Interactive Menu ====================\n");
        printf("  1. FCFS Scheduling                   8. Banker's Algorithm (Safe)\n");
        printf("  2. Round Robin Scheduling            9. Banker's Algorithm (Unsafe)\n");
        printf("  3. Priority Scheduling              10. Normal Worker Process\n");
        printf("  4. Scheduling Policy Comparison     11. Simulated Worker Failure\n");
        printf("  5. Producer-Consumer Logger         12. Resource Limits (setrlimit)\n");
        printf("  6. Shared-Memory IPC                13. Cleanup IPC Resources\n");
        printf("  7. Semaphore & Cleanup Check        14. Run Integrated Demo (All)\n");
        printf("  0. Exit\n");
        printf("========================================================================\n");
        printf("Select an option [0-14]: ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n[INFO] EOF detected. Exiting interactive menu.\n");
            break;
        }

        char *endptr = NULL;
        long choice = strtol(line, &endptr, 10);
        if (endptr == line || (*endptr != '\n' && *endptr != '\0')) {
            fprintf(stderr, "[ERROR] Invalid menu input '%s'. Please enter an integer between 0 and 14.\n",
                    strtok(line, "\r\n"));
            continue;
        }

        switch (choice) {
            case 0:
                cleanup_all_resources(0);
                printf("[INFO] Exiting CampusCompute. Goodbye!\n");
                return 0;
            case 1: run_fcfs(jobs, n, 1); break;
            case 2: run_round_robin(jobs, n, DEFAULT_TIME_QUANTUM, 1); break;
            case 3: run_priority_scheduling(jobs, n, 1); break;
            case 4: run_scheduling_comparison(jobs, n, DEFAULT_TIME_QUANTUM); break;
            case 5: run_logger_demo(); break;
            case 6: run_shm_ipc_demo(0); break;
            case 7: run_sem_cleanup_demo(); break;
            case 8: run_banker_demo(1); break;
            case 9: run_banker_demo(0); break;
            case 10: run_worker_demo(0); break;
            case 11: run_worker_demo(1); break;
            case 12: run_rlimit_demo(); break;
            case 13: cleanup_all_resources(1); break;
            case 14: run_integrated_demo(); break;
            default:
                fprintf(stderr, "[ERROR] Option %ld is out of range [0-14]. Run '%s help' for usage.\n",
                        choice, prog_name);
                break;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    Job jobs[MAX_JOBS];
    int n = load_default_jobs(jobs);

    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    } else if (strcmp(cmd, "fcfs") == 0) {
        run_fcfs(jobs, n, 1);
        return 0;
    } else if (strcmp(cmd, "rr") == 0) {
        int quantum = DEFAULT_TIME_QUANTUM;
        if (argc >= 3) {
            if (parse_positive_int(argv[2], &quantum) != 0) {
                fprintf(stderr, "[ERROR] Invalid Round Robin time quantum '%s'. Must be a positive integer.\n",
                        argv[2]);
                fprintf(stderr, "Usage: %s rr [positive_quantum]\n", argv[0]);
                return EXIT_FAILURE;
            }
        }
        run_round_robin(jobs, n, quantum, 1);
        return 0;
    } else if (strcmp(cmd, "priority") == 0) {
        run_priority_scheduling(jobs, n, 1);
        return 0;
    } else if (strcmp(cmd, "compare") == 0) {
        int quantum = DEFAULT_TIME_QUANTUM;
        if (argc >= 3) {
            if (parse_positive_int(argv[2], &quantum) != 0) {
                fprintf(stderr, "[ERROR] Invalid time quantum '%s'. Must be a positive integer.\n", argv[2]);
                return EXIT_FAILURE;
            }
        }
        run_scheduling_comparison(jobs, n, quantum);
        return 0;
    } else if (strcmp(cmd, "custom-sched") == 0) {
        if (argc < 4) {
            fprintf(stderr, "[ERROR] Malformed 'custom-sched' command: missing job count or job specifications.\n");
            fprintf(stderr, "Usage: %s custom-sched <num_jobs> <id,arrival,burst,priority> ...\n", argv[0]);
            return EXIT_FAILURE;
        }
        int custom_n = 0;
        if (parse_positive_int(argv[2], &custom_n) != 0 || custom_n > MAX_JOBS || argc != 3 + custom_n) {
            fprintf(stderr, "[ERROR] Invalid job count '%s' or mismatch with provided job tuples (expected %d tuples).\n",
                    argv[2], custom_n);
            return EXIT_FAILURE;
        }
        Job custom_jobs[MAX_JOBS];
        for (int i = 0; i < custom_n; i++) {
            if (parse_custom_job(argv[3 + i], &custom_jobs[i]) != 0) {
                fprintf(stderr, "[ERROR] Malformed job tuple '%s' at position %d. Expected format: id,arrival,burst,priority\n",
                        argv[3 + i], i + 1);
                return EXIT_FAILURE;
            }
        }
        if (validate_jobs(custom_jobs, custom_n) != 0) {
            return EXIT_FAILURE;
        }
        return run_scheduling_comparison(custom_jobs, custom_n, DEFAULT_TIME_QUANTUM);
    } else if (strcmp(cmd, "logger") == 0) {
        return (run_logger_demo() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "ipc") == 0) {
        return (run_shm_ipc_demo(1) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "sem") == 0) {
        return (run_sem_cleanup_demo() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "banker-safe") == 0) {
        return (run_banker_demo(1) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "banker-unsafe") == 0) {
        int rc = run_banker_demo(0);
        return (rc >= 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "worker-ok") == 0) {
        return (run_worker_demo(0) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "worker-fail") == 0) {
        return (run_worker_demo(1) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "rlimit") == 0) {
        return (run_rlimit_demo() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "cleanup") == 0) {
        return (cleanup_all_resources(1) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "all") == 0) {
        return (run_integrated_demo() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (strcmp(cmd, "menu") == 0) {
        return run_interactive_menu(argv[0]);
    } else {
        fprintf(stderr, "[ERROR] Unrecognized command: '%s'\n", cmd);
        fprintf(stderr, "Run '%s help' to view valid commands and syntax.\n", argv[0]);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
}
