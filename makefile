CC	= gcc
CFLAGS = -g
TARGET1	= oss
TARGET2	= user_proc
OBJS1	= oss.o
OBJS2	= user_proc.o

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) -o $@ $(OBJS1) $(LIBS) -lm

$(TARGET2): $(OBJS2)
	$(CC) -o $@ $(OBJS2) $(LIBS) -lm

.SUFFIXES: .c .o

.c.o:
	$(CC) -c $(CFLAGS) $<

.PHONY: clean

clean:
	/bin/rm -f core *.o $(TARGET1) $(TARGET2) *.out output.log
