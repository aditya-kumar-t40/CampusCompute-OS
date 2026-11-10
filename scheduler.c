/*
 * scheduler.c
 * FR1: Job Management, Input Validation, and Process Lifecycle Tracking
 * FR2: CPU Scheduling (FCFS, Round Robin, Non-Preemptive Priority Scheduling)
 */

#include "campus_compute.h"

const char* job_status_str(JobStatus status) {
    switch (status) {
        case JOB_READY:     return "READY";
        case JOB_RUNNING:   return "RUNNING";
        case JOB_WAITING:   return "WAITING";
        case JOB_COMPLETED: return "COMPLETED";
        case JOB_FAILED:    return "FAILED";
        default:            return "UNKNOWN";
    }
}

/* Validate job parameters (FR1) */
int validate_jobs(const Job *jobs, int n) {
    if (jobs == NULL || n <= 0 || n > MAX_JOBS) {
        fprintf(stderr, "[ERROR] Invalid job count (%d). Must be between 1 and %d.\n", n, MAX_JOBS);
        return -1;
    }
    for (int i = 0; i < n; i++) {
        if (jobs[i].job_id <= 0) {
            fprintf(stderr, "[ERROR] Validation failed: Job at index %d has invalid job_id (%d). Must be > 0.\n",
                    i, jobs[i].job_id);
            return -1;
        }
        if (jobs[i].arrival_time < 0) {
            fprintf(stderr, "[ERROR] Validation failed: Job J%d has negative arrival_time (%d).\n",
                    jobs[i].job_id, jobs[i].arrival_time);
            return -1;
        }
        if (jobs[i].cpu_burst <= 0) {
            fprintf(stderr, "[ERROR] Validation failed: Job J%d has non-positive cpu_burst (%d).\n",
                    jobs[i].job_id, jobs[i].cpu_burst);
            return -1;
        }
        if (jobs[i].priority < 0) {
            fprintf(stderr, "[ERROR] Validation failed: Job J%d has negative priority (%d).\n",
                    jobs[i].job_id, jobs[i].priority);
            return -1;
        }
        for (int j = i + 1; j < n; j++) {
            if (jobs[i].job_id == jobs[j].job_id) {
                fprintf(stderr, "[ERROR] Validation failed: Duplicate job_id (%d) detected.\n", jobs[i].job_id);
                return -1;
            }
        }
    }
    return 0;
}

/* Load deterministic repeatable campus computing workload (FR1 & FR2) */
int load_default_jobs(Job *jobs) {
    Job preset[5] = {
        {1, "ML_Train_Sim",    0, 6, 6, 2, -1, 0, 0, 0, JOB_READY},
        {2, "CFD_Fluid_Sim",   1, 4, 4, 1, -1, 0, 0, 0, JOB_READY},
        {3, "Matrix_Solver",   2, 5, 5, 3, -1, 0, 0, 0, JOB_READY},
        {4, "Genomics_Align",  3, 2, 2, 1, -1, 0, 0, 0, JOB_READY},
        {5, "Quantum_Circuit", 4, 3, 3, 2, -1, 0, 0, 0, JOB_READY}
    };
    memcpy(jobs, preset, sizeof(preset));
    return 5;
}

/* Print initial or completed job table */
void print_job_table(const Job *jobs, int n, const char *title) {
    printf("\n====================================================================================\n");
    printf(" %s\n", title);
    printf("====================================================================================\n");
    printf(" %-6s %-16s %-8s %-7s %-8s %-10s %-10s %-8s %-10s\n",
           "JobID", "JobName", "Arrival", "Burst", "Priority", "Start", "Complete", "TAT", "Waiting");
    printf("------------------------------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf(" J%-5d %-16s %-8d %-7d %-8d %-10d %-10d %-8d %-10d [%s]\n",
               jobs[i].job_id,
               jobs[i].job_name,
               jobs[i].arrival_time,
               jobs[i].cpu_burst,
               jobs[i].priority,
               jobs[i].start_time,
               jobs[i].completion_time,
               jobs[i].turnaround_time,
               jobs[i].waiting_time,
               job_status_str(jobs[i].status));
    }
    printf("====================================================================================\n");
}

/*
 * First-Come, First-Served (FCFS) Scheduling (FR2)
 * Tie-breaking rule: Lower Arrival Time first; if equal, lower Job ID first.
 */
ScheduleSummary run_fcfs(const Job *input_jobs, int n, int verbose) {
    ScheduleSummary summary = {"FCFS", 0.0, 0.0, 0};
    if (validate_jobs(input_jobs, n) != 0) {
        return summary;
    }

    Job jobs[MAX_JOBS];
    memcpy(jobs, input_jobs, sizeof(Job) * n);

    /* Sort by arrival_time, then job_id */
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            int swap = 0;
            if (jobs[j].arrival_time > jobs[j + 1].arrival_time) {
                swap = 1;
            } else if (jobs[j].arrival_time == jobs[j + 1].arrival_time &&
                       jobs[j].job_id > jobs[j + 1].job_id) {
                swap = 1;
            }
            if (swap) {
                Job tmp = jobs[j];
                jobs[j] = jobs[j + 1];
                jobs[j + 1] = tmp;
            }
        }
    }

    if (verbose) {
        printf("\n[FCFS SCHEDULER] Policy: First-Come, First-Served (Non-Preemptive)\n");
        printf("[FCFS SCHEDULER] Tie-Breaking Rule: Lower Arrival Time first; if tied, lower Job ID.\n");
        printf("[FCFS SCHEDULER] Execution Timeline (Gantt Chart):\n  ");
    }

    int current_time = 0;
    double total_tat = 0.0;
    double total_wt = 0.0;

    for (int i = 0; i < n; i++) {
        if (current_time < jobs[i].arrival_time) {
            if (verbose) {
                printf("| [%d..%d] IDLE ", current_time, jobs[i].arrival_time);
            }
            current_time = jobs[i].arrival_time;
        }
        jobs[i].status = JOB_RUNNING;
        jobs[i].start_time = current_time;
        current_time += jobs[i].cpu_burst;
        jobs[i].remaining_time = 0;
        jobs[i].completion_time = current_time;
        jobs[i].turnaround_time = jobs[i].completion_time - jobs[i].arrival_time;
        jobs[i].waiting_time = jobs[i].turnaround_time - jobs[i].cpu_burst;
        jobs[i].status = JOB_COMPLETED;

        total_tat += jobs[i].turnaround_time;
        total_wt += jobs[i].waiting_time;

        if (verbose) {
            printf("| [%d..%d] J%d ", jobs[i].start_time, jobs[i].completion_time, jobs[i].job_id);
        }
    }

    if (verbose) {
        printf("|\n");
        print_job_table(jobs, n, "FCFS Scheduling Results & Job Lifecycle Status");
        printf(" -> Average Turnaround Time (TAT): %.2f units\n", total_tat / n);
        printf(" -> Average Waiting Time (WT)    : %.2f units\n", total_wt / n);
    }

    summary.avg_turnaround_time = total_tat / n;
    summary.avg_waiting_time = total_wt / n;
    summary.total_completion_time = current_time;
    return summary;
}

/*
 * Round Robin (RR) Scheduling (FR2)
 * Tie-breaking rule: Standard FIFO ready queue; newly arriving jobs at time <= t are
 * enqueued in order of Arrival Time (then lower Job ID) before re-queueing preempted job.
 */
ScheduleSummary run_round_robin(const Job *input_jobs, int n, int quantum, int verbose) {
    ScheduleSummary summary = {"Round Robin", 0.0, 0.0, 0};
    if (quantum <= 0) {
        fprintf(stderr, "[ERROR] Round Robin time quantum must be > 0 (got %d).\n", quantum);
        return summary;
    }
    if (validate_jobs(input_jobs, n) != 0) {
        return summary;
    }

    Job jobs[MAX_JOBS];
    memcpy(jobs, input_jobs, sizeof(Job) * n);

    /* Sort by arrival_time, then job_id */
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (jobs[j].arrival_time > jobs[j + 1].arrival_time ||
               (jobs[j].arrival_time == jobs[j + 1].arrival_time && jobs[j].job_id > jobs[j + 1].job_id)) {
                Job tmp = jobs[j];
                jobs[j] = jobs[j + 1];
                jobs[j + 1] = tmp;
            }
        }
    }

    int ready_q[MAX_JOBS * 16];
    int q_head = 0, q_tail = 0;
    int in_queue[MAX_JOBS] = {0};
    int completed = 0;
    int current_time = 0;

    /* Seed initial arrivals at time 0 */
    for (int i = 0; i < n; i++) {
        if (jobs[i].arrival_time <= current_time) {
            ready_q[q_tail++] = i;
            in_queue[i] = 1;
        }
    }

    if (verbose) {
        printf("\n[ROUND ROBIN SCHEDULER] Policy: Preemptive Round Robin (Time Quantum = %d units)\n", quantum);
        printf("[ROUND ROBIN SCHEDULER] Tie-Breaking Rule: FIFO Ready Queue; arrivals at <= current time enqueued by Arrival Time, then Job ID, before preempted job re-enters.\n");
        printf("[ROUND ROBIN SCHEDULER] Execution Timeline (Gantt Chart):\n  ");
    }

    while (completed < n) {
        if (q_head == q_tail) {
            /* CPU is idle; jump to next arrival */
            int next_arrival = -1;
            for (int i = 0; i < n; i++) {
                if (jobs[i].status != JOB_COMPLETED) {
                    if (next_arrival == -1 || jobs[i].arrival_time < jobs[next_arrival].arrival_time) {
                        next_arrival = i;
                    }
                }
            }
            if (next_arrival != -1) {
                if (verbose) {
                    printf("| [%d..%d] IDLE ", current_time, jobs[next_arrival].arrival_time);
                }
                current_time = jobs[next_arrival].arrival_time;
                for (int i = 0; i < n; i++) {
                    if (!in_queue[i] && jobs[i].status != JOB_COMPLETED && jobs[i].arrival_time <= current_time) {
                        ready_q[q_tail++] = i;
                        in_queue[i] = 1;
                    }
                }
            }
            continue;
        }

        int idx = ready_q[q_head++];
        if (jobs[idx].start_time == -1) {
            jobs[idx].start_time = current_time;
        }
        jobs[idx].status = JOB_RUNNING;

        int slice = (jobs[idx].remaining_time < quantum) ? jobs[idx].remaining_time : quantum;
        int slice_start = current_time;
        current_time += slice;
        jobs[idx].remaining_time -= slice;

        if (verbose) {
            printf("| [%d..%d] J%d ", slice_start, current_time, jobs[idx].job_id);
        }

        /* Enqueue any jobs that arrived during this time slice */
        for (int i = 0; i < n; i++) {
            if (!in_queue[i] && jobs[i].status != JOB_COMPLETED && jobs[i].arrival_time <= current_time) {
                ready_q[q_tail++] = i;
                in_queue[i] = 1;
            }
        }

        if (jobs[idx].remaining_time == 0) {
            jobs[idx].status = JOB_COMPLETED;
            jobs[idx].completion_time = current_time;
            jobs[idx].turnaround_time = jobs[idx].completion_time - jobs[idx].arrival_time;
            jobs[idx].waiting_time = jobs[idx].turnaround_time - jobs[idx].cpu_burst;
            completed++;
        } else {
            jobs[idx].status = JOB_WAITING;
            ready_q[q_tail++] = idx;
        }
    }

    double total_tat = 0.0, total_wt = 0.0;
    for (int i = 0; i < n; i++) {
        total_tat += jobs[i].turnaround_time;
        total_wt += jobs[i].waiting_time;
    }

    if (verbose) {
        printf("|\n");
        print_job_table(jobs, n, "Round Robin Scheduling Results & Job Lifecycle Status");
        printf(" -> Time Quantum                 : %d units\n", quantum);
        printf(" -> Average Turnaround Time (TAT): %.2f units\n", total_tat / n);
        printf(" -> Average Waiting Time (WT)    : %.2f units\n", total_wt / n);
    }

    summary.avg_turnaround_time = total_tat / n;
    summary.avg_waiting_time = total_wt / n;
    summary.total_completion_time = current_time;
    return summary;
}

/*
 * Non-Preemptive Priority Scheduling (FR2)
 * Priority convention: Lower priority number = higher urgency (e.g., Priority 1 > Priority 2).
 * Tie-breaking rule: Among ready jobs with identical priority, select earlier Arrival Time;
 * if still tied, select lower Job ID.
 */
ScheduleSummary run_priority_scheduling(const Job *input_jobs, int n, int verbose) {
    ScheduleSummary summary = {"Priority (Non-Preemptive)", 0.0, 0.0, 0};
    if (validate_jobs(input_jobs, n) != 0) {
        return summary;
    }

    Job jobs[MAX_JOBS];
    memcpy(jobs, input_jobs, sizeof(Job) * n);

    if (verbose) {
        printf("\n[PRIORITY SCHEDULER] Policy: Non-Preemptive Priority Scheduling (1 = Highest Priority)\n");
        printf("[PRIORITY SCHEDULER] Tie-Breaking Rule: Equal Priority -> Earlier Arrival Time -> Lower Job ID.\n");
        printf("[PRIORITY SCHEDULER] Execution Timeline (Gantt Chart):\n  ");
    }

    int completed = 0;
    int current_time = 0;
    double total_tat = 0.0, total_wt = 0.0;

    while (completed < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (jobs[i].status != JOB_COMPLETED && jobs[i].arrival_time <= current_time) {
                if (best == -1) {
                    best = i;
                } else if (jobs[i].priority < jobs[best].priority) {
                    best = i;
                } else if (jobs[i].priority == jobs[best].priority) {
                    if (jobs[i].arrival_time < jobs[best].arrival_time ||
                       (jobs[i].arrival_time == jobs[best].arrival_time && jobs[i].job_id < jobs[best].job_id)) {
                        best = i;
                    }
                }
            }
        }

        if (best == -1) {
            /* Advance to earliest uncompleted job arrival */
            int earliest = -1;
            for (int i = 0; i < n; i++) {
                if (jobs[i].status != JOB_COMPLETED) {
                    if (earliest == -1 || jobs[i].arrival_time < jobs[earliest].arrival_time) {
                        earliest = i;
                    }
                }
            }
            if (verbose) {
                printf("| [%d..%d] IDLE ", current_time, jobs[earliest].arrival_time);
            }
            current_time = jobs[earliest].arrival_time;
            continue;
        }

        jobs[best].status = JOB_RUNNING;
        jobs[best].start_time = current_time;
        current_time += jobs[best].cpu_burst;
        jobs[best].remaining_time = 0;
        jobs[best].completion_time = current_time;
        jobs[best].turnaround_time = jobs[best].completion_time - jobs[best].arrival_time;
        jobs[best].waiting_time = jobs[best].turnaround_time - jobs[best].cpu_burst;
        jobs[best].status = JOB_COMPLETED;

        total_tat += jobs[best].turnaround_time;
        total_wt += jobs[best].waiting_time;
        completed++;

        if (verbose) {
            printf("| [%d..%d] J%d(P%d) ", jobs[best].start_time, jobs[best].completion_time,
                   jobs[best].job_id, jobs[best].priority);
        }
    }

    if (verbose) {
        printf("|\n");
        print_job_table(jobs, n, "Priority Scheduling Results & Job Lifecycle Status");
        printf(" -> Average Turnaround Time (TAT): %.2f units\n", total_tat / n);
        printf(" -> Average Waiting Time (WT)    : %.2f units\n", total_wt / n);
    }

    summary.avg_turnaround_time = total_tat / n;
    summary.avg_waiting_time = total_wt / n;
    summary.total_completion_time = current_time;
    return summary;
}

/* Compare all 3 scheduling policies on the exact same job set (FR2 / S09) */
int run_scheduling_comparison(const Job *input_jobs, int n, int quantum) {
    if (validate_jobs(input_jobs, n) != 0) {
        return -1;
    }

    printf("\n====================================================================================\n");
    printf(" SCHEDULING POLICY COMPARISON (Dataset: %d Campus Compute Jobs, RR Quantum = %d)\n", n, quantum);
    printf("====================================================================================\n");

    ScheduleSummary s_fcfs = run_fcfs(input_jobs, n, 1);
    ScheduleSummary s_rr   = run_round_robin(input_jobs, n, quantum, 1);
    ScheduleSummary s_prio = run_priority_scheduling(input_jobs, n, 1);

    printf("\n====================================================================================\n");
    printf(" SUMMARY COMPARISON TABLE ACROSS SCHEDULING POLICIES\n");
    printf("====================================================================================\n");
    printf(" %-28s | %-18s | %-18s | %-12s\n",
           "Scheduling Policy", "Avg Turnaround", "Avg Waiting Time", "Total Span");
    printf("------------------------------------------------------------------------------------\n");
    printf(" %-28s | %-18.2f | %-18.2f | %-12d\n",
           s_fcfs.policy_name, s_fcfs.avg_turnaround_time, s_fcfs.avg_waiting_time, s_fcfs.total_completion_time);
    printf(" %-28s | %-18.2f | %-18.2f | %-12d\n",
           s_rr.policy_name, s_rr.avg_turnaround_time, s_rr.avg_waiting_time, s_rr.total_completion_time);
    printf(" %-28s | %-18.2f | %-18.2f | %-12d\n",
           s_prio.policy_name, s_prio.avg_turnaround_time, s_prio.avg_waiting_time, s_prio.total_completion_time);
    printf("====================================================================================\n");

    char log_buf[160];
    snprintf(log_buf, sizeof(log_buf),
             "Scheduler comparison completed: FCFS(WT=%.2f), RR_Q%d(WT=%.2f), Priority(WT=%.2f)",
             s_fcfs.avg_waiting_time, quantum, s_rr.avg_waiting_time, s_prio.avg_waiting_time);
    append_direct_log("SCHEDULER", log_buf);
    return 0;
}
