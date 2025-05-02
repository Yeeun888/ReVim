#Directories
IDIR = ../include
LDIR = ../lib
ODIR = obj

CXX=g++
CPPFLAGS= --std=c++17 -Wall -I$(IDIR) -lncurses
# for this project -lncurses flag is required 

CC=gcc
CFLAGS= -I$(IDIR)

LIBS=-lm

_DEPS = 
DEPS = $(patsubst %,$(IDIR)/%, $(_DEPS))

_OBJ = revim.o gap_buffer.o
OBJS = $(patsubst %,$(ODIR)/%, $(_OBJ))

$(ODIR)/%.o : %.cpp $(DEPS) 
	$(CXX) -c -o $@ $< $(CPPFLAGS)

$(ODIR)/%.o : %.c $(DEPS) 
	$(CC) -c -o $@ $< $(CFLAGS)

revim: $(OBJS)
	$(CXX) -o $@ $^ $(CPPFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~

# Design Notes
# 1. Using .o files helps gcc generate object files such that when dependencies
# change e.g. .h files not everything has to be recompiled. 

