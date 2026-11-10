# CampusCompute: Intelligent Job Scheduling and Resource Management

**Course:** Operating Systems (UE25CS242A) — Level 3 Mini-Project  
**Platform:** Linux / Ubuntu or WSL Ubuntu (C11, POSIX Threads, POSIX Shared Memory & Named Semaphores)

## 1. Prerequisites

Ensure `gcc`, `make`, and `zip` are installed on Ubuntu / WSL Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential zip
```

## 2. Project Structure

| File | Description |
| :--- | :--- |
| `campus_compute.h` | Shared data structures (`Job`, `ScheduleSummary`, `BoundedLogQueue`, `SharedClusterState`), constants, and prototypes |
| `main.c` | Command-line interface, interactive menu, input validation, error handling, and integrated workflow (`FR8`) |
| `scheduler.c` | Job validation, lifecycle tracking (`FR1`), FCFS, Round Robin, Non-Preemptive Priority, and policy comparison (`FR2`) |
| `logger.c` | Bounded producer-consumer activity logger using `pthread_t`, `pthread_mutex_t`, and `pthread_cond_t` (`FR3`) |
| `ipc_worker.c` | POSIX `shm_open`/`mmap` + named semaphore `sem_open` (`FR4`), `fork`/`waitpid` worker monitoring (`FR6`), `setrlimit` & cleanup (`FR7`) |
| `banker.c` | Banker's Safety Algorithm (`Need = Maximum - Allocation`) demonstrating both SAFE and UNSAFE states (`FR5`) |
| `Makefile` | Build and cleanup rules using `gcc -Wall -Wextra -Werror -std=c11 -O2 -pthread` |

## 3. Build and Cleanup Commands

- **Clean & Build from source:**
  ```bash
  make clean && make
  ```
- **Clean up IPC resources & build artifacts:**
  ```bash
  ./campus_compute cleanup
  make clean
  ```
  `make clean` removes only local build/runtime files. Use the application's
  `cleanup` command to unlink the IPC objects reserved by CampusCompute; the
  IPC demo uses exclusive creation and will not overwrite an existing object.

## 4. Validation performed

The project was rebuilt with the strict Makefile flags (`-Wall -Wextra -Werror`)
and the following command paths were exercised successfully in a Linux test
environment: `help`, `fcfs`, `rr 2`, `priority`, `compare 2`, `custom-sched`,
`logger`, `ipc`, `sem`, `banker-safe`, `banker-unsafe`, `worker-ok`,
`worker-fail`, `rlimit`, `cleanup`, and `all`. Invalid Round Robin quantum and
invalid custom-job burst inputs were also checked and rejected.

Run the same tests in your own Ubuntu/WSL environment before capturing your
genuine terminal screenshots. Kernel-dependent resource-limit output can vary
between Linux environments.

## 5. Step-by-Step Commands for Screenshots (S01 – S24)

> **Important Rule:** Before taking any screenshots, set your own complete SRN in your terminal prompt (replace `PES1UG25CS001` below with your actual SRN):
> ```bash
> export SRN='YOUR_FULL_SRN'
> export PS1='[$SRN] \w\$ '
> ```

| Screenshot ID | Requirement | Exact Command to Run in Terminal | Suggested 1-Line Explanation for PDF |
| :--- | :--- | :--- | :--- |
| **S01** | Environment & SRN prompt | `export SRN='YOUR_FULL_SRN'; export PS1='[$SRN] \w\$ '; uname -a; pwd` | Demonstrates the Ubuntu/WSL terminal environment configured with the full student SRN prompt and project working directory. |
| **S02** | Compiler & build tools | `gcc --version | head -n 1 && make --version | head -n 1 && zip -v | head -n 2` | Confirms that GCC, GNU Make, and ZIP build tools are installed and available on the Linux system. |
| **S03** | Project files | `ls -la campus_compute.h main.c scheduler.c logger.c banker.c ipc_worker.c Makefile README.md` | Displays the project directory containing all modular C source files, header file, Makefile, and documentation. |
| **S04** | Clean build | `make clean && make` | Shows the CampusCompute prototype compiling cleanly from source with zero warnings or errors using strict C11 flags. |
| **S05** | CLI help/usage | `./campus_compute help` | Displays the command-line help menu listing all independently testable operating-system commands and options. |
| **S06** | FCFS scheduling | `./campus_compute fcfs` | Demonstrates First-Come, First-Served (FCFS) scheduling with execution timeline, tie-breaking rule, and turnaround/waiting averages. |
| **S07** | Round Robin scheduling | `./campus_compute rr 2` | Shows preemptive Round Robin scheduling with time quantum = 2, Gantt timeline, and calculated per-job metrics. |
| **S08** | Priority scheduling | `./campus_compute priority` | Demonstrates non-preemptive Priority Scheduling (1 = highest priority), tie-breaking rule, and turnaround/waiting times. |
| **S09** | Scheduling comparison | `./campus_compute compare 2` | Compares FCFS, Round Robin (Q=2), and Priority Scheduling side-by-side on the identical 5-job workload. |
| **S10** | Producer-consumer logger | `./campus_compute logger` | Shows 3 POSIX producer threads and 1 consumer thread operating on a bounded circular log queue of capacity 5. |
| **S11** | Thread sync & termination | `./campus_compute logger | grep -E "SYNC-WAIT|THREAD-EXIT|LOGGER SUMMARY"` | Verifies mutex and condition-variable synchronization (`not_full`/`not_empty`) and clean termination of all threads. |
| **S12** | Shared-memory IPC | `./campus_compute ipc` | Shows cooperating parent and child processes exchanging cluster telemetry via POSIX shared memory (`shm_open` and `mmap`). |
| **S13** | Semaphore & cleanup | `./campus_compute sem` | Demonstrates mutual exclusion via POSIX named semaphore (`sem_open`) and verifies unlinking of shared IPC objects. |
| **S14** | Banker's safe state | `./campus_compute banker-safe` | Evaluates a safe resource state using Banker's algorithm (`Need = Max - Alloc`) and outputs the safe sequence `<P1 -> P3 -> P4 -> P0 -> P2>`. |
| **S15** | Banker's unsafe state | `./campus_compute banker-unsafe` | Identifies an unsafe resource state where remaining resource needs exceed available work and prevents deadlock. |
| **S16** | Normal worker process | `./campus_compute worker-ok` | Shows child worker creation via `fork()` and parent collection of normal exit status (`0`) via `waitpid()`. |
| **S17** | Simulated worker failure | `./campus_compute worker-fail` | Demonstrates parent-side detection of controlled simulated worker failures (non-zero exit code 42 and `SIGUSR1` termination). |
| **S18** | Resource-limit demo | `./campus_compute rlimit` | Applies `RLIMIT_FSIZE` (256 B) and `RLIMIT_CPU` (1 s) via `setrlimit()` and shows kernel enforcement (`EFBIG` and `SIGKILL`/`SIGXCPU`). |
| **S19** | Invalid input/command | `./campus_compute invalid_cmd ; ./campus_compute rr -5 ; ./campus_compute custom-sched 2 1,0,-3,1 2,1,4,2` | Verifies input validation and helpful error/usage messages for unrecognized commands, negative time quantum, and invalid job bursts. |
| **S20** | Cleanup verification | `./campus_compute cleanup && ls -la /dev/shm` | Confirms all child processes have terminated and CampusCompute-created shared memory and named semaphores are removed from `/dev/shm`. |
| **S21** | Integrated execution | `./campus_compute all` | Executes the complete integrated demonstration across all six subsystems and prints the final verification summary. |
| **S22** | Generated logs | `ls -lh campus_compute.log && cat campus_compute.log` | Displays the generated `campus_compute.log` file and its timestamped producer-consumer and subsystem event entries. |
| **S23** | Final source & build files | `ls -la` | Shows the final compiled executable `campus_compute` alongside all C source files, header, Makefile, and log file. |
| **S24** | Submission ZIP | `zip -r CampusCompute_Submission.zip Makefile README.md campus_compute.h *.c && unzip -l CampusCompute_Submission.zip && zip -T CampusCompute_Submission.zip` | Creates the submission ZIP archive, lists its contents, and confirms archive integrity (`test of CampusCompute_Submission.zip OK`). |
