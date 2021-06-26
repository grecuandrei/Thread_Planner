CPP = gcc
CFLAGS = -fPIC -Wall -g -c

build: libscheduler.so

so_scheduler.o: so_scheduler.c
	$(CPP) $(CFLAGS) $^ -o $@

queue.o: queue.c
	$(CPP) $(CFLAGS) $^ -o $@

libscheduler.so: so_scheduler.o queue.o
	$(CPP) -shared $^ -o $@

.PHONY: clean
clean:
	-rm -f so_scheduler.o libscheduler.so queue.o