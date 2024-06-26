ROOT = ../..

BINS = $(BINDIR)/counter-cas


include $(ROOT)/common/Makefile.common

PROF = $(ROOT)/src

.PHONY:	all clean

all:	main 

measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

counter-cas.o: 
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/counter-cas.o counter-cas.c

test.o: counter-cas.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

main: measurements.o ssalloc.o  counter-cas.o test.o 
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/counter-cas.o  $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
