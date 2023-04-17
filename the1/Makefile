CC = gcc
CFLAGS = -Wall -g

INCDIR = include
SRCDIR = src
BINDIR = bin

_OBJ = main.o message.o utils.o logging.o
OBJ = $(patsubst %,$(SRCDIR)/%,$(_OBJ))

INPUT_FILE ?= input2.txt

all: $(BINDIR)/bgame run

run: $(BINDIR)/bgame
	$< < $(INPUT_FILE)

$(BINDIR)/bgame: $(OBJ)
	@mkdir -p $(BINDIR)
	$(CC) -o $@ $^ $(CFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCDIR)

.PHONY: clean

clean:
	rm -f $(SRCDIR)/*.o
