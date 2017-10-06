CFLAGS =-g
CC = gcc

PROG= /usr/local/bin/change_time_step
OBJS = main.o change_time_step.o change_time_step_tsd.o

LPATHS = -L/usr/X11R6/lib  -L/usr/local/lib
IPATHS = -I/usr/local/include  

$(PROG) : $(OBJS)
	$(CC) $(CFLAGS) $(LPATHS) $(IPATHS)  $(OBJS) -o $(PROG) -lm -lc -laoaux -lrand; rm -f *.o


.c.o:
	$(CC) $(CFLAGS) $(IPATHS) -c $*.c

