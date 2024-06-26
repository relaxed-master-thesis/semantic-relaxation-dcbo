ROOT = ../..

include $(ROOT)/common/Makefile.common

ifeq ($(HEURISTIC),LENGTH)
	CFLAGS += -DLENGTH_HEURISTIC
	BINS = $(BINDIR)/dcbl-wfqueue
else
	BINS = $(BINDIR)/dcbo-wfqueue
endif

ifeq ($(TEST), BFS)
	TEST_FILE = test-bfs.c
endif

PROF = $(ROOT)/src

.PHONY:	all clean

all:	main

measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

partial-wfqueue.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/partial-wfqueue.o partial-wfqueue.c

d-balanced-queue.o: partial-wfqueue.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/d-balanced-queue.o d-balanced-queue.c

test.o: d-balanced-queue.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: measurements.o ssalloc.o  d-balanced-queue.o test.o partial-wfqueue.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/d-balanced-queue.o $(BUILDIR)/partial-wfqueue.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)