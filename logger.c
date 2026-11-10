/*
 * logger.c
 * FR3: Concurrent Activity Logger using POSIX Threads, Mutex, and Condition Variables
 * Implements a bounded producer-consumer circular queue that writes timestamped logs
 * to campus_compute.log and terminates worker threads cleanly.
 */

#include "campus_compute.h"

#define NUM_PRODUCERS 3
#define EVENTS_PER_PRODUCER 4

static BoundedLogQueue g_log_queue;
static FILE *g_log_fp = NULL;

/* Format current timestamp for readable event logs */
static void get_timestamp(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm_info);
}

/* Helper to append a single log entry directly from other modules */
int append_direct_log(const char *tag, const char *message) {
    FILE *fp = fopen(LOG_FILE_PATH, "a");
    if (!fp) {
        return -1;
    }
    char ts[32];
    get_timestamp(ts, sizeof(ts));
    fprintf(fp, "[%s] [%s] %s\n", ts, tag, message);
    fclose(fp);
    return 0;
}

/* Initialize bounded queue, mutex, and condition variables */
static int log_queue_init(BoundedLogQueue *q) {
    memset(q, 0, sizeof(*q));
    if (pthread_mutex_init(&q->mutex, NULL) != 0) return -1;
    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        pthread_cond_destroy(&q->not_full);
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    return 0;
}

/* Destroy synchronization primitives */
static void log_queue_destroy(BoundedLogQueue *q) {
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    pthread_mutex_destroy(&q->mutex);
}

/* Producer thread function: generates job/resource activity events */
static void* producer_routine(void *arg) {
    int producer_id = *(int*)arg;
    const char *stage_descriptions[EVENTS_PER_PRODUCER] = {
        "Submitted compute job to ready queue",
        "Allocated GPU/CPU node slice and verified safety",
        "Completed simulation epoch checkpoint",
        "Released compute node locks and finalized job metrics"
    };

    for (int i = 0; i < EVENTS_PER_PRODUCER; i++) {
        char event_msg[LOG_MSG_LEN];
        char ts[32];
        get_timestamp(ts, sizeof(ts));
        snprintf(event_msg, sizeof(event_msg),
                 "[%s] [PRODUCER-T%d] Event %d/%d: %s",
                 ts, producer_id, i + 1, EVENTS_PER_PRODUCER, stage_descriptions[i]);

        pthread_mutex_lock(&g_log_queue.mutex);

        while (g_log_queue.count == LOG_QUEUE_CAPACITY && !g_log_queue.shutdown) {
            printf("  [SYNC-WAIT] Producer-T%d waiting on 'not_full' condvar (Queue Full: %d/%d)\n",
                   producer_id, g_log_queue.count, LOG_QUEUE_CAPACITY);
            pthread_cond_wait(&g_log_queue.not_full, &g_log_queue.mutex);
        }

        if (g_log_queue.shutdown) {
            pthread_mutex_unlock(&g_log_queue.mutex);
            break;
        }

        snprintf(g_log_queue.entries[g_log_queue.tail], LOG_MSG_LEN, "%s", event_msg);
        g_log_queue.tail = (g_log_queue.tail + 1) % LOG_QUEUE_CAPACITY;
        g_log_queue.count++;

        printf("  [PRODUCE]   Producer-T%d enqueued event %d/%d (Queue occupancy: %d/%d)\n",
               producer_id, i + 1, EVENTS_PER_PRODUCER, g_log_queue.count, LOG_QUEUE_CAPACITY);

        pthread_cond_signal(&g_log_queue.not_empty);
        pthread_mutex_unlock(&g_log_queue.mutex);

        /* Brief stagger so producer/consumer interleaving is clear */
        usleep(15000);
    }

    printf("  [THREAD-EXIT] Producer-T%d finished producing all events and exited cleanly.\n", producer_id);
    return NULL;
}

/* Consumer thread function: drains bounded queue and writes to log file */
static void* consumer_routine(void *arg) {
    (void)arg;
    int consumed_total = 0;

    while (1) {
        pthread_mutex_lock(&g_log_queue.mutex);

        while (g_log_queue.count == 0 && !g_log_queue.shutdown) {
            printf("  [SYNC-WAIT] Consumer-Logger waiting on 'not_empty' condvar (Queue Empty: 0/%d)\n",
                   LOG_QUEUE_CAPACITY);
            pthread_cond_wait(&g_log_queue.not_empty, &g_log_queue.mutex);
        }

        if (g_log_queue.count == 0 && g_log_queue.shutdown) {
            pthread_mutex_unlock(&g_log_queue.mutex);
            break;
        }

        char msg[LOG_MSG_LEN];
        snprintf(msg, sizeof(msg), "%s", g_log_queue.entries[g_log_queue.head]);
        g_log_queue.head = (g_log_queue.head + 1) % LOG_QUEUE_CAPACITY;
        g_log_queue.count--;
        consumed_total++;

        printf("  [CONSUME]   Consumer-Logger dequeued entry #%02d (Queue occupancy: %d/%d) -> Writing to %s\n",
               consumed_total, g_log_queue.count, LOG_QUEUE_CAPACITY, LOG_FILE_PATH);

        pthread_cond_signal(&g_log_queue.not_full);
        pthread_mutex_unlock(&g_log_queue.mutex);

        if (g_log_fp) {
            fprintf(g_log_fp, "%s\n", msg);
            fflush(g_log_fp);
        }
        usleep(25000);
    }

    printf("  [THREAD-EXIT] Consumer-Logger received shutdown signal, drained queue (%d entries), and exited cleanly.\n",
           consumed_total);
    return NULL;
}

/* Run full Producer-Consumer Concurrent Logger demonstration (FR3 / S10 & S11) */
int run_logger_demo(void) {
    printf("\n====================================================================================\n");
    printf(" CONCURRENT ACTIVITY LOGGER (POSIX Threads, Mutex & Condition Variables)\n");
    printf(" Bounded Queue Capacity: %d | Producer Threads: %d | Events/Producer: %d | Log: %s\n",
           LOG_QUEUE_CAPACITY, NUM_PRODUCERS, EVENTS_PER_PRODUCER, LOG_FILE_PATH);
    printf("====================================================================================\n");

    if (log_queue_init(&g_log_queue) != 0) {
        perror("[ERROR] Failed to initialize bounded log queue");
        return -1;
    }

    g_log_fp = fopen(LOG_FILE_PATH, "a");
    if (!g_log_fp) {
        perror("[ERROR] Failed to open log file for writing");
        log_queue_destroy(&g_log_queue);
        return -1;
    }

    char ts[32];
    get_timestamp(ts, sizeof(ts));
    fprintf(g_log_fp, "[%s] [LOGGER-INIT] Bounded producer-consumer logger session started (cap=%d)\n",
            ts, LOG_QUEUE_CAPACITY);
    fflush(g_log_fp);

    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumer;
    int producer_ids[NUM_PRODUCERS];

    if (pthread_create(&consumer, NULL, consumer_routine, NULL) != 0) {
        perror("[ERROR] Failed to create consumer thread");
        fclose(g_log_fp);
        log_queue_destroy(&g_log_queue);
        return -1;
    }

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i + 1;
        if (pthread_create(&producers[i], NULL, producer_routine, &producer_ids[i]) != 0) {
            perror("[ERROR] Failed to create producer thread");
            pthread_mutex_lock(&g_log_queue.mutex);
            g_log_queue.shutdown = 1;
            pthread_cond_broadcast(&g_log_queue.not_empty);
            pthread_mutex_unlock(&g_log_queue.mutex);
            for (int j = 0; j < i; j++) pthread_join(producers[j], NULL);
            pthread_join(consumer, NULL);
            fclose(g_log_fp);
            log_queue_destroy(&g_log_queue);
            return -1;
        }
    }

    /* Wait for all producer threads to complete */
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    /* Signal clean shutdown to consumer thread and join */
    pthread_mutex_lock(&g_log_queue.mutex);
    g_log_queue.shutdown = 1;
    pthread_cond_broadcast(&g_log_queue.not_empty);
    pthread_mutex_unlock(&g_log_queue.mutex);

    pthread_join(consumer, NULL);

    get_timestamp(ts, sizeof(ts));
    fprintf(g_log_fp, "[%s] [LOGGER-STOP] All %d producer threads and 1 consumer thread joined cleanly.\n",
            ts, NUM_PRODUCERS);
    fclose(g_log_fp);
    g_log_fp = NULL;

    log_queue_destroy(&g_log_queue);

    printf("------------------------------------------------------------------------------------\n");
    printf(" [LOGGER SUMMARY] Total events produced & logged: %d | Queue residual count: %d\n",
           NUM_PRODUCERS * EVENTS_PER_PRODUCER, g_log_queue.count);
    printf(" [LOGGER SUMMARY] Mutex and Condition Variables destroyed; log saved to '%s'.\n",
           LOG_FILE_PATH);
    printf("====================================================================================\n");
    return 0;
}
