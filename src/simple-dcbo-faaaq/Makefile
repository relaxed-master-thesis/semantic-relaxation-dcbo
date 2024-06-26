ROOT = ../..

ifeq ($(HEURISTIC),LENGTH)
	CFLAGS += -DLENGTH_HEURISTIC
	BINS = $(BINDIR)/simple-dcbl-faaaq
else
	BINS = $(BINDIR)/simple-dcbo-faaaq
endif


include $(ROOT)/common/Makefile.common

PROF = $(ROOT)/src

.PHONY:	all clean

all:	main

measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

partial-faaaq.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/partial-faaaq.o partial-faaaq.c

external-count-wrapper.o: partial-faaaq.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/external-count-wrapper.o external-count-wrapper.c

d-balanced-queue.o: external-count-wrapper.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/d-balanced-queue.o d-balanced-queue.c

test.o: d-balanced-queue.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: measurements.o ssalloc.o  d-balanced-queue.o test.o partial-faaaq.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/d-balanced-queue.o $(BUILDIR)/partial-faaaq.o $(BUILDIR)/external-count-wrapper.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
