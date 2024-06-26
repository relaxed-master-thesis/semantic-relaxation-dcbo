ROOT = ../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/queue-wf-ssmem

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

wfqueue-ssmem.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/wfqueue-ssmem.o wfqueue-ssmem.c

test.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: measurements.o ssalloc.o  test.o wfqueue-ssmem.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/wfqueue-ssmem.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)