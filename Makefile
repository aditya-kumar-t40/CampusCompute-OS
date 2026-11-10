# Makefile for CampusCompute: Intelligent Job Scheduling and Resource Management
# Operating Systems (UE25CS242A) - Level 3 Mini-Project

CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -std=c11 -O2 -pthread
LDFLAGS := -pthread -lrt

TARGET  := campus_compute
SRCS    := main.c scheduler.c logger.c banker.c ipc_worker.c
OBJS    := $(SRCS:.c=.o)
HDRS    := campus_compute.h

.PHONY: all clean run-all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

run-all: $(TARGET)
	./$(TARGET) all

clean:
	rm -f $(OBJS) $(TARGET) campus_compute.log rlimit_test_scratch.tmp
