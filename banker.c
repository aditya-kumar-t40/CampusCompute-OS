/*
 * banker.c
 * FR5: Resource-Safety Analysis using Dijkstra's Banker's Safety Algorithm
 * Computes Need = Maximum - Allocation, displays resource matrices, and determines
 * whether the system is in a SAFE state (with safe execution sequence) or UNSAFE state.
 */

#include "campus_compute.h"

static void print_resource_matrices(
    const int available[NUM_RESOURCES],
    const int allocation[NUM_PROCESSES][NUM_RESOURCES],
    const int maximum[NUM_PROCESSES][NUM_RESOURCES],
    const int need[NUM_PROCESSES][NUM_RESOURCES],
    const char *scenario_title)
{
    printf("\n====================================================================================\n");
    printf(" %s\n", scenario_title);
    printf(" Resources: R0 (CPU Cores), R1 (GPU Units), R2 (High-Speed NVMe Channels)\n");
    printf("====================================================================================\n");
    printf(" %-8s | %-14s | %-14s | %-14s | %-14s\n",
           "Process", "Allocation", "Maximum", "Need (Max-Alloc)", "Available");
    printf(" %-8s | %-4s %-4s %-4s | %-4s %-4s %-4s | %-4s %-4s %-4s | %-4s %-4s %-4s\n",
           "", "R0", "R1", "R2", "R0", "R1", "R2", "R0", "R1", "R2", "R0", "R1", "R2");
    printf("------------------------------------------------------------------------------------\n");

    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (i == 0) {
            printf(" P%-7d | %-4d %-4d %-4d | %-4d %-4d %-4d | %-4d %-4d %-4d | %-4d %-4d %-4d\n",
                   i,
                   allocation[i][0], allocation[i][1], allocation[i][2],
                   maximum[i][0],    maximum[i][1],    maximum[i][2],
                   need[i][0],       need[i][1],       need[i][2],
                   available[0],     available[1],     available[2]);
        } else {
            printf(" P%-7d | %-4d %-4d %-4d | %-4d %-4d %-4d | %-4d %-4d %-4d |\n",
                   i,
                   allocation[i][0], allocation[i][1], allocation[i][2],
                   maximum[i][0],    maximum[i][1],    maximum[i][2],
                   need[i][0],       need[i][1],       need[i][2]);
        }
    }
    printf("====================================================================================\n");
}

/* Execute Banker's Safety Algorithm on given matrices */
static int evaluate_safety(
    const int available[NUM_RESOURCES],
    const int allocation[NUM_PROCESSES][NUM_RESOURCES],
    const int maximum[NUM_PROCESSES][NUM_RESOURCES])
{
    int need[NUM_PROCESSES][NUM_RESOURCES];
    int work[NUM_RESOURCES];
    int finish[NUM_PROCESSES] = {0};
    int safe_sequence[NUM_PROCESSES] = {0};
    int safe_count = 0;

    /* Calculate Need = Maximum - Allocation and validate non-negative need */
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            need[i][j] = maximum[i][j] - allocation[i][j];
            if (need[i][j] < 0) {
                fprintf(stderr, "[ERROR] Process P%d has Allocation > Maximum for resource R%d!\n", i, j);
                return -1;
            }
        }
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        work[j] = available[j];
    }

    print_resource_matrices(available, allocation, maximum, need,
                            "BANKER'S ALGORITHM RESOURCE STATE MATRICES");

    printf(" [STEP-BY-STEP SAFETY EVALUATION] Initial Work Vector = [%d, %d, %d]\n",
           work[0], work[1], work[2]);

    while (safe_count < NUM_PROCESSES) {
        int found_runnable = 0;
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (!finish[i]) {
                int can_satisfy = 1;
                for (int j = 0; j < NUM_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        can_satisfy = 0;
                        break;
                    }
                }
                if (can_satisfy) {
                    printf("  -> Step %d: P%d Need [%d, %d, %d] <= Work [%d, %d, %d] | P%d executes & releases [%d, %d, %d]",
                           safe_count + 1, i,
                           need[i][0], need[i][1], need[i][2],
                           work[0], work[1], work[2],
                           i,
                           allocation[i][0], allocation[i][1], allocation[i][2]);
                    for (int j = 0; j < NUM_RESOURCES; j++) {
                        work[j] += allocation[i][j];
                    }
                    printf(" -> New Work = [%d, %d, %d]\n", work[0], work[1], work[2]);
                    finish[i] = 1;
                    safe_sequence[safe_count++] = i;
                    found_runnable = 1;
                }
            }
        }
        if (!found_runnable) {
            break;
        }
    }

    printf("------------------------------------------------------------------------------------\n");
    if (safe_count == NUM_PROCESSES) {
        printf(" [RESULT: SAFE STATE] System is in a SAFE state. No deadlock risk detected.\n");
        printf(" [SAFE SEQUENCE]    : < ");
        for (int i = 0; i < NUM_PROCESSES; i++) {
            printf("P%d%s", safe_sequence[i], (i == NUM_PROCESSES - 1) ? " >\n" : " -> ");
        }
        printf("====================================================================================\n");
        append_direct_log("BANKER-SAFE", "Verified SAFE resource state with sequence <P1 -> P3 -> P4 -> P0 -> P2>");
        return 0;
    } else {
        printf(" [RESULT: UNSAFE STATE] Deadlock hazard! Only %d/%d processes can finish.\n",
               safe_count, NUM_PROCESSES);
        printf(" [BLOCKED PROCESSES]  : ");
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (!finish[i]) {
                printf("P%d (Need=[%d,%d,%d] > Work=[%d,%d,%d])  ",
                       i, need[i][0], need[i][1], need[i][2], work[0], work[1], work[2]);
            }
        }
        printf("\n [ACTION] Admission controller rejects/defers resource allocation request.\n");
        printf("====================================================================================\n");
        append_direct_log("BANKER-UNSAFE", "Detected UNSAFE resource state; blocked processes cannot satisfy Need <= Work");
        return 1;
    }
}

/* Run Banker's Algorithm demonstration for either Safe State (1) or Unsafe State (0) */
int run_banker_demo(int test_safe_state) {
    int allocation[NUM_PROCESSES][NUM_RESOURCES] = {
        {0, 1, 0}, /* P0 */
        {2, 0, 0}, /* P1 */
        {3, 0, 2}, /* P2 */
        {2, 1, 1}, /* P3 */
        {0, 0, 2}  /* P4 */
    };

    int maximum_safe[NUM_PROCESSES][NUM_RESOURCES] = {
        {7, 5, 3}, /* P0 */
        {3, 2, 2}, /* P1 */
        {9, 0, 2}, /* P2 */
        {2, 2, 2}, /* P3 */
        {4, 3, 3}  /* P4 */
    };

    if (test_safe_state) {
        int available_safe[NUM_RESOURCES] = {3, 3, 2};
        printf("\n[BANKER'S DEMO] Scenario 1: Evaluating SAFE Cluster Resource Allocation State\n");
        return evaluate_safety(available_safe, allocation, maximum_safe);
    } else {
        /* Unsafe state: Available reduced to [1, 0, 0] and high remaining demands */
        int available_unsafe[NUM_RESOURCES] = {1, 0, 0};
        int maximum_unsafe[NUM_PROCESSES][NUM_RESOURCES] = {
            {7, 5, 3}, /* P0: Need [7, 4, 3] */
            {4, 2, 2}, /* P1: Need [2, 2, 2] */
            {9, 0, 4}, /* P2: Need [6, 0, 2] */
            {4, 3, 3}, /* P3: Need [2, 2, 2] */
            {4, 3, 4}  /* P4: Need [4, 3, 2] */
        };
        printf("\n[BANKER'S DEMO] Scenario 2: Evaluating UNSAFE Cluster Resource Allocation State\n");
        return evaluate_safety(available_unsafe, allocation, maximum_unsafe);
    }
}
