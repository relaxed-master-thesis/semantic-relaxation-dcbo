ROOT = ../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/lcrq

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

lcrq.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/lcrq.o lcrq.c

test.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: test.o ssalloc.o lcrq.o measurements.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/test.o $(BUILDIR)/lcrq.o $(BUILDIR)/ssalloc.o -o $(BINS) $(LDFLAGS)
clean:
	-rm -f $(BINS)