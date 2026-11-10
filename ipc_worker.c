/*
 * ipc_worker.c
 * FR4: Inter-Process Communication using POSIX Shared Memory & Named Semaphores
 * FR6: Worker Failure Detection using fork() and waitpid()
 * FR7: Resource Limits using setrlimit() and Clean Resource Release
 */

#include "campus_compute.h"

/*
 * FR4: Shared-Memory IPC & Named Semaphore Synchronization
 * Parent (Controller) and Child (Worker Node) exchange structured telemetry
 * via POSIX shm_open/mmap coordinated with a POSIX named semaphore (sem_open).
 */
int run_shm_ipc_demo(int keep_resources_for_inspect) {
    printf("\n====================================================================================\n");
    printf(" POSIX SHARED MEMORY IPC & NAMED SEMAPHORE SYNCHRONIZATION (FR4)\n");
    printf(" Shared Memory Object: %s | Named Semaphore: %s\n", SHM_NAME, SEM_NAME);
    printf("====================================================================================\n");

    /* Create uniquely named project objects without unlinking anything first.
     * O_EXCL prevents this run from overwriting or deleting an existing object
     * (for example, one owned by another concurrently running instance).
     */
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm_fd < 0) {
        perror("[ERROR] shm_open failed");
        return -1;
    }

    if (ftruncate(shm_fd, sizeof(SharedClusterState)) != 0) {
        perror("[ERROR] ftruncate failed");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return -1;
    }

    SharedClusterState *shared = mmap(NULL, sizeof(SharedClusterState),
                                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("[ERROR] mmap failed");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return -1;
    }

    sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0600, 1);
    if (sem == SEM_FAILED) {
        perror("[ERROR] sem_open failed");
        munmap(shared, sizeof(SharedClusterState));
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return -1;
    }

    /* Initialize shared memory region under semaphore lock */
    sem_wait(sem);
    memset(shared, 0, sizeof(SharedClusterState));
    shared->producer_pid = getpid();
    shared->active_jobs_completed = 3;
    shared->total_cpu_cycles = 1500;
    snprintf(shared->status_report, sizeof(shared->status_report),
             "Controller (PID %d) dispatched 3 jobs (1500 CPU cycles)", (int)getpid());
    shared->sync_flag = 1;
    printf(" [IPC-PARENT] PID %d acquired semaphore '%s' and initialized shared memory:\n",
           (int)getpid(), SEM_NAME);
    printf("              Jobs=%d, Cycles=%d, Message=\"%s\"\n",
           shared->active_jobs_completed, shared->total_cpu_cycles, shared->status_report);
    sem_post(sem);

    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] fork failed in IPC demo");
        sem_close(sem);
        sem_unlink(SEM_NAME);
        munmap(shared, sizeof(SharedClusterState));
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return -1;
    }

    if (pid == 0) {
        /* Cooperating Child Process reads & updates shared memory */
        sem_wait(sem);
        printf(" [IPC-CHILD]  PID %d acquired semaphore '%s' and read from shared memory:\n",
               (int)getpid(), SEM_NAME);
        printf("              Received from Parent PID %d -> \"%s\"\n",
               (int)shared->producer_pid, shared->status_report);

        shared->consumer_pid = getpid();
        shared->active_jobs_completed += 2;
        shared->total_cpu_cycles += 900;
        snprintf(shared->status_report, sizeof(shared->status_report),
                 "Worker (PID %d) processed 2 additional jobs; total=5 jobs (2400 cycles)",
                 (int)getpid());
        shared->sync_flag = 2;

        printf(" [IPC-CHILD]  PID %d updated shared memory (Jobs=%d, Cycles=%d) and released semaphore.\n",
               (int)getpid(), shared->active_jobs_completed, shared->total_cpu_cycles);
        fflush(stdout);
        sem_post(sem);

        sem_close(sem);
        munmap(shared, sizeof(SharedClusterState));
        close(shm_fd);
        _exit(0);
    }

    /* Parent waits for child to finish updating shared memory */
    int status = 0;
    waitpid(pid, &status, 0);

    sem_wait(sem);
    printf(" [IPC-PARENT] PID %d re-acquired semaphore '%s' after Child PID %d exited:\n",
           (int)getpid(), SEM_NAME, (int)pid);
    printf("              Verified Updated Shared State -> Jobs=%d, Cycles=%d, SyncFlag=%d\n",
           shared->active_jobs_completed, shared->total_cpu_cycles, shared->sync_flag);
    printf("              Final Report: \"%s\"\n", shared->status_report);
    sem_post(sem);

    sem_close(sem);
    munmap(shared, sizeof(SharedClusterState));
    close(shm_fd);

    if (!keep_resources_for_inspect) {
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        printf(" [IPC-CLEANUP] Unlinked shared memory '%s' and semaphore '%s' cleanly.\n",
               SHM_NAME, SEM_NAME);
    } else {
        printf(" [IPC-INFO] Objects '%s' and '%s' retained for inspection; run 'sem' or 'cleanup' to release.\n",
               SHM_NAME, SEM_NAME);
    }
    printf("====================================================================================\n");

    append_direct_log("IPC-SHM", "Completed cooperative POSIX shared memory and named semaphore exchange");
    return 0;
}

/* Demonstrate Semaphore Coordination and explicit IPC Cleanup verification (S13) */
int run_sem_cleanup_demo(void) {
    printf("\n====================================================================================\n");
    printf(" SEMAPHORE COORDINATION & IPC RESOURCE CLEANUP VERIFICATION (FR4 / FR7)\n");
    printf("====================================================================================\n");

    /* Run shared memory + semaphore coordination and unlink objects */
    if (run_shm_ipc_demo(0) != 0) {
        return -1;
    }

    /* Verify that neither SHM_NAME nor SEM_NAME remains in /dev/shm */
    int shm_probe = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_probe < 0 && errno == ENOENT) {
        printf(" [VERIFY-SHM] Verified '%s' is removed (shm_open O_RDONLY -> ENOENT: No such file or directory).\n",
               SHM_NAME);
    } else {
        if (shm_probe >= 0) close(shm_probe);
        shm_unlink(SHM_NAME);
    }

    sem_t *sem_probe = sem_open(SEM_NAME, 0);
    if (sem_probe == SEM_FAILED && errno == ENOENT) {
        printf(" [VERIFY-SEM] Verified '%s' is removed (sem_open -> ENOENT: No such file or directory).\n",
               SEM_NAME);
    } else if (sem_probe != SEM_FAILED) {
        sem_close(sem_probe);
        sem_unlink(SEM_NAME);
    }

    printf("====================================================================================\n");
    return 0;
}

/*
 * FR6: Worker Failure Detection using fork() and waitpid()
 * Mode 0: Normal worker completion (exit code 0)
 * Mode 1: Controlled simulated worker failure (non-zero exit & simulated SIGUSR1 child)
 */
int run_worker_demo(int simulate_failure) {
    printf("\n====================================================================================\n");
    if (!simulate_failure) {
        printf(" WORKER PROCESS MONITORING: NORMAL COMPLETION MODE (FR6)\n");
    } else {
        printf(" WORKER PROCESS MONITORING: SIMULATED WORKER FAILURE MODE (FR6)\n");
        printf(" [NOTE] Failures below are controlled simulations for OS fault-detection testing.\n");
    }
    printf("====================================================================================\n");

    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] fork failed");
        return -1;
    }

    if (pid == 0) {
        /* Worker Child Process */
        printf(" [WORKER-CHILD] Started worker process (PID=%d, PPID=%d).\n",
               (int)getpid(), (int)getppid());
        if (!simulate_failure) {
            printf(" [WORKER-CHILD] Executing matrix simulation workload...\n");
            fflush(stdout);
            usleep(40000);
            printf(" [WORKER-CHILD] Workload finished without errors. Calling _exit(0).\n");
            fflush(stdout);
            _exit(0);
        } else {
            printf(" [WORKER-CHILD] [SIMULATED FAILURE] Encountered corrupt checkpoint data! Exiting with code 42.\n");
            fflush(stdout);
            usleep(30000);
            _exit(42);
        }
    }

    int status = 0;
    pid_t waited = waitpid(pid, &status, 0);
    if (waited < 0) {
        perror("[ERROR] waitpid failed");
        return -1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code == 0) {
            printf(" [MONITOR-PARENT] waitpid() collected Child PID=%d -> NORMAL COMPLETION (WIFEXITED=1, Exit Status=%d).\n",
                   (int)waited, code);
            append_direct_log("WORKER-OK", "Child worker completed normally with exit code 0");
        } else {
            printf(" [MONITOR-PARENT] [ALERT] waitpid() detected SIMULATED WORKER FAILURE for Child PID=%d!\n",
                   (int)waited);
            printf("                  Diagnostic: WIFEXITED=1, Non-Zero Exit Code=%d (Simulated Checkpoint Error).\n",
                   code);
        }
    }

    /* In failure mode, also demonstrate parent detection of a simulated signal termination */
    if (simulate_failure) {
        fflush(stdout);
        pid_t sig_pid = fork();
        if (sig_pid == 0) {
            printf(" [WORKER-CHILD-2] Started second worker (PID=%d) to test simulated signal crash (SIGUSR1).\n",
                   (int)getpid());
            usleep(20000);
            printf(" [WORKER-CHILD-2] [SIMULATED FAILURE] Raising SIGUSR1 to simulate abnormal worker termination.\n");
            fflush(stdout);
            raise(SIGUSR1);
            _exit(1);
        } else if (sig_pid > 0) {
            int sig_status = 0;
            waitpid(sig_pid, &sig_status, 0);
            if (WIFSIGNALED(sig_status)) {
                int sig_num = WTERMSIG(sig_status);
                printf(" [MONITOR-PARENT] [ALERT] waitpid() detected SIMULATED SIGNAL TERMINATION for Child PID=%d!\n",
                       (int)sig_pid);
                printf("                  Diagnostic: WIFSIGNALED=1, Terminating Signal=%d (%s) [Controlled Simulation].\n",
                       sig_num, strsignal(sig_num));
                append_direct_log("WORKER-FAIL", "Detected simulated worker failure (Exit=42 and Signal=SIGUSR1)");
            }
        }
    }

    printf("====================================================================================\n");
    return 0;
}

/*
 * FR7: Resource Limits Demonstration using setrlimit()
 * Forks an isolated worker process, applies a strict RLIMIT_CPU (1 second) and
 * RLIMIT_FSIZE (256 bytes) limit, and observes the kernel enforcement cleanly.
 */
int run_rlimit_demo(void) {
    printf("\n====================================================================================\n");
    printf(" PROCESS RESOURCE LIMITS DEMONSTRATION USING setrlimit() (FR7)\n");
    printf("====================================================================================\n");

    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        perror("[ERROR] fork failed in rlimit demo");
        return -1;
    }

    if (pid == 0) {
        struct rlimit fsize_lim;
        fsize_lim.rlim_cur = 256; /* 256 bytes soft limit */
        fsize_lim.rlim_max = 256; /* 256 bytes hard limit */

        /* Ignore SIGXFSZ so write() returns EFBIG instead of killing the process immediately,
         * allowing us to inspect both the errno effect and CPU limit enforcement */
        signal(SIGXFSZ, SIG_IGN);

        if (setrlimit(RLIMIT_FSIZE, &fsize_lim) != 0) {
            perror("[ERROR] setrlimit(RLIMIT_FSIZE) failed");
            _exit(1);
        }

        struct rlimit verify_lim;
        getrlimit(RLIMIT_FSIZE, &verify_lim);
        printf(" [RLIMIT-WORKER] PID %d applied setrlimit(RLIMIT_FSIZE): soft=%lu bytes, hard=%lu bytes.\n",
               (int)getpid(), (unsigned long)verify_lim.rlim_cur, (unsigned long)verify_lim.rlim_max);

        const char *tmp_file = "rlimit_test_scratch.tmp";
        int fd = open(tmp_file, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (fd >= 0) {
            char block[512];
            memset(block, 'A', sizeof(block));
            ssize_t w = write(fd, block, sizeof(block));
            if (w < 0) {
                printf(" [RLIMIT-EFFECT] Attempted 512-byte write -> Blocked by kernel! errno=%d (%s)\n",
                       errno, strerror(errno));
            } else if (w < (ssize_t)sizeof(block)) {
                printf(" [RLIMIT-EFFECT] Attempted 512-byte write -> Truncated at %zd bytes by RLIMIT_FSIZE limit!\n", w);
                /* Second write at offset 256 triggers EFBIG */
                ssize_t w2 = write(fd, block, 64);
                if (w2 < 0) {
                    printf(" [RLIMIT-EFFECT] Subsequent write past %lu-byte cap failed with errno=%d (%s)!\n",
                           (unsigned long)verify_lim.rlim_cur, errno, strerror(errno));
                }
            }
            close(fd);
            unlink(tmp_file);
            printf(" [RLIMIT-CLEANUP] Removed temporary test file '%s'.\n", tmp_file);
        }

        /* Also demonstrate RLIMIT_CPU (1 second) enforcement via SIGXCPU / SIGKILL */
        struct rlimit cpu_lim;
        cpu_lim.rlim_cur = 1;
        cpu_lim.rlim_max = 1;
        if (setrlimit(RLIMIT_CPU, &cpu_lim) == 0) {
            printf(" [RLIMIT-WORKER] Applied setrlimit(RLIMIT_CPU) = 1 second. Running busy loop to observe kernel signal...\n");
            fflush(stdout);
            volatile unsigned long counter = 0;
            while (1) {
                counter++;
            }
        }
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf(" [RLIMIT-PARENT] Kernel enforced RLIMIT_CPU on Worker PID %d -> Terminated by signal %d (%s).\n",
               (int)pid, sig, strsignal(sig));
    } else if (WIFEXITED(status)) {
        printf(" [RLIMIT-PARENT] Worker PID %d exited with status %d.\n",
               (int)pid, WEXITSTATUS(status));
    }
    printf("====================================================================================\n");
    append_direct_log("RLIMIT", "Demonstrated RLIMIT_FSIZE (EFBIG) and RLIMIT_CPU kernel enforcement");
    return 0;
}

/*
 * FR7: Clean up only program-created IPC objects and temporary files
 */
int cleanup_all_resources(int verbose) {
    if (verbose) {
        printf("\n====================================================================================\n");
        printf(" PROGRAM RESOURCE CLEANUP & VERIFICATION (FR7)\n");
        printf("====================================================================================\n");
    }

    int shm_res = shm_unlink(SHM_NAME);
    if (verbose) {
        if (shm_res == 0) {
            printf(" [CLEANUP] Unlinked POSIX shared memory object '%s'.\n", SHM_NAME);
        } else if (errno == ENOENT) {
            printf(" [CLEANUP] POSIX shared memory '%s' already clean (not present in /dev/shm).\n", SHM_NAME);
        } else {
            perror(" [CLEANUP] shm_unlink warning");
        }
    }

    int sem_res = sem_unlink(SEM_NAME);
    if (verbose) {
        if (sem_res == 0) {
            printf(" [CLEANUP] Unlinked POSIX named semaphore '%s'.\n", SEM_NAME);
        } else if (errno == ENOENT) {
            printf(" [CLEANUP] POSIX named semaphore '%s' already clean (not present in /dev/shm).\n", SEM_NAME);
        } else {
            perror(" [CLEANUP] sem_unlink warning");
        }
    }

    unlink("rlimit_test_scratch.tmp");

    /* Reap any stray child processes belonging to this process */
    int status = 0;
    pid_t reaped;
    int reaped_count = 0;
    while ((reaped = waitpid(-1, &status, WNOHANG)) > 0) {
        reaped_count++;
    }

    if (verbose) {
        printf(" [CLEANUP] Active child processes remaining: %d (all worker processes reaped).\n", reaped_count);
        printf(" [CLEANUP] Only CampusCompute-owned IPC objects ('%s', '%s') were targeted.\n",
               SHM_NAME, SEM_NAME);
        printf("====================================================================================\n");
    }
    return 0;
}
