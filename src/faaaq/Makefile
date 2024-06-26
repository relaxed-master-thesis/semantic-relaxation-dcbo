ROOT = ../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/faaaq

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

faaaq.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/faaaq.o faaaq.c

test.o: faaaq.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: test.o ssalloc.o faaaq.o measurements.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/test.o $(BUILDIR)/faaaq.o $(BUILDIR)/ssalloc.o -o $(BINS) $(LDFLAGS)
clean:
	-rm -f $(BINS)