ROOT = ../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/ms

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

ms.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ms.o ms.c

test.o: ms.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: test.o ssalloc.o ms.o measurements.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/test.o $(BUILDIR)/ms.o $(BUILDIR)/ssalloc.o -o $(BINS) $(LDFLAGS)
clean:
	-rm -f $(BINS)