ROOT = ../..

include $(ROOT)/common/Makefile.common

ifeq ($(HEURISTIC),LENGTH)
	CFLAGS += -DLENGTH_HEURISTIC
	BINS = $(BINDIR)/dcbl-ms
else
	BINS = $(BINDIR)/dcbo-ms
endif

ifeq ($(TEST), BFS)
	TEST_FILE = test-bfs.c
endif

PROF = $(ROOT)/src

.PHONY:    all clean

all:    main

measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

partial-ms.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/partial-ms.o partial-ms.c

d-balanced-queue.o: partial-ms.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/d-balanced-queue.o d-balanced-queue.c

test.o: d-balanced-queue.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: test.o ssalloc.o d-balanced-queue.o partial-ms.o measurements.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/test.o $(BUILDIR)/partial-ms.o $(BUILDIR)/ssalloc.o $(BUILDIR)/d-balanced-queue.o -o $(BINS) $(LDFLAGS)
clean:
	-rm -f $(BINS)