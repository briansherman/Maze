SHELL	=  /bin/sh

SRC = maze.c \

OUT = $(SRC:%.c=%)

PROF	= #-pg
DBG	= -g
OPT	= -O #-O6
WARN	= #-Wall -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings \
#	-Waggregate-return -Winline -Werror

CC	= gcc
CLINK	= gcc

INC	= 
LIB	= -lglut -lGLU -lGL

CFLAGS	= $(PROF) $(INC) $(DBG) $(WARN) $(OPT)
CLNKFLGS= $(PROF) $(DBG) $(WARN) $(OPT)

all:	$(SRC) $(OUT) Makefile
.PHONY : all

%:	%.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

%.c:	%_patch
	(/usr/bin/patch -i $^ -o $@)

clean:
	/bin/rm -f $(OUT)
.PHONY : clean
