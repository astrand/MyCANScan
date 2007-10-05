BIN	=	canvis
CC	=	cc
INCDIR	=	-I/usr/local/include -I/usr/include -I/usr/X11R6/include
CFLAGS	=	-g -Wa,
LDFLAGS	=	$(MKSYM) -L/usr/lib -L/usr/local/lib -L/lib -L/usr/X11R6/lib -lX11 -lm -lpng

OBJS	=	main.o\
		graphics.o\
		functions.o\
		fileio.o\

.c.o:
	$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

$(BIN):	$(OBJS) 
	$(CC) $(OBJS) $(LDFLAGS) -o $(BIN)

clean:
	rm -f core *.o $(BIN) $(OBJS)
