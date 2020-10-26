SRCDIR = src
HEADERDIR = include
OBJDIR = obj

CC = gcc
CFLAGS = -g -Wall -Wextra -Ofast -I$(HEADERDIR)
LFLAGS = -pthread

SOURCES := $(shell find $(SRCDIR) -name '*.c')
OBJECTS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

MAINS := $(OBJDIR)/master.o $(OBJDIR)/worker.o $(OBJDIR)/client.o $(OBJDIR)/server.o

OBJ := $(filter-out $(MAINS), $(OBJECTS))


all: master worker whoClient whoServer

master: $(OBJDIR) $(OBJECTS)
	$(CC) $(OBJ) $(OBJDIR)/master.o -o $@

worker: $(OBJDIR) $(OBJECTS)
	$(CC) $(OBJ) $(OBJDIR)/worker.o -o $@

whoClient: $(OBJDIR) $(OBJECTS)
	$(CC) $(OBJ) $(OBJDIR)/client.o -o $@ $(LFLAGS)

whoServer: $(OBJDIR) $(OBJECTS)
	$(CC) $(OBJ) $(OBJDIR)/server.o -o $@ $(LFLAGS)


$(OBJDIR):
	@mkdir $(OBJDIR)


$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LFLAGS)



.PHONY: clean

clean:
	rm -rf fifo.* master worker whoClient whoServer $(OBJDIR)/*