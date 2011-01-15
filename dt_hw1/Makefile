
CC = gcc
LIBS = -lm
FLAGS = -O2
EXEC = dt
SRCFILES = auxi.c dt.c entropy.c main.c print-dt.c prune-dt.c ssv.c
OBJFILES = auxi.o dt.o entropy.o main.o print-dt.o prune-dt.o ssv.o

all: $(EXEC)
	@echo ""
	@echo "Compilation Done"
	@echo ""

clean:
	rm -f *.o *~ $(EXEC)

$(EXEC): $(OBJFILES)
	$(CC) $(POST_PRUNING) -g -o $(EXEC) $(OBJFILES) $(LIBS)

%.o: %.c
	$(CC) -c $(POST_PRUNING) -g -o $(*F).o $(FLAGS) $(*F).c
