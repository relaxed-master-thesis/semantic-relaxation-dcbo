ROOT = ../..

ifeq ($(HEURISTIC),LENGTH)
	CFLAGS += -DLENGTH_HEURISTIC
	BINS = $(BINDIR)/simple-dcbl-lcrq
else
	BINS = $(BINDIR)/simple-dcbo-lcrq
endif

include $(ROOT)/common/Makefile.common

PROF = $(ROOT)/src

.PHONY:	all clean

all:	main

measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

lcrq.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/lcrq.o lcrq.c

external-count-wrapper.o: lcrq.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/external-count-wrapper.o external-count-wrapper.c

d-balanced-queue.o: external-count-wrapper.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/d-balanced-queue.o d-balanced-queue.c

test.o: d-balanced-queue.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: measurements.o lcrq.o test.o ssalloc.o d-balanced-queue.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/d-balanced-queue.o $(BUILDIR)/lcrq.o $(BUILDIR)/ssalloc.o $(BUILDIR)/test.o $(BUILDIR)/external-count-wrapper.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
