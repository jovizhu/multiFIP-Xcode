#!/bin/sh
#
# Makefile for FF v 1.0
#


####### FLAGS

TYPE	= 
ADDONS	= 

CC      = gcc 

#CFLAGS	= -O6 -Wall -g -ansi $(TYPE) $(ADDONS) 
ifeq ($(UNAME),Darwin) # Mac OS
CFLAGS = -O3 -g -stdlib=libstdc++ -ansi $(TYPE) $(ADDONS) 
endif

ifeq ($(UNAME), Linux) # Linux
CFLAGS = -O6 -g -w -ansi $(TYPE) $(ADDONS) 
endif
# -g -pg

LIBS    = -lm -lstdc++


####### Files


PDDL_PARSER_SRC	= scan-fct_pddl.tab.c \
	scan-ops_pddl.tab.c \
	scan-mul_pddl.tab.c \
	scan-probname.tab.c \
	lex.fct_pddl.c \
	lex.ops_pddl.c \
	lex.mul_pddl.c

PDDL_PARSER_OBJ = scan-fct_pddl.tab.o \
	scan-ops_pddl.tab.o \
	scan-mul_pddl.tab.o


SOURCES 	= main.c \
	memory.c \
	output.c \
	parse.c \
	inst_pre.c \
	inst_easy.c \
	inst_hard.c \
	inst_final.c \
	orderings.c \
	relax.c \
	search.c \
	fip.c

OBJECTS 	= $(SOURCES:.c=.o)

####### Implicit rules

.SUFFIXES:

.SUFFIXES: .c .o

.c.o:; $(CC) -c $(CFLAGS) $<

####### Build rules

all: $(OBJECTS) $(PDDL_PARSER_OBJ)
	$(CC) -o fip $(OBJECTS) $(PDDL_PARSER_OBJ) $(CFLAGS) $(LIBS)
	
	
fip: $(OBJECTS) $(PDDL_PARSER_OBJ)
	$(CC) -o fip $(OBJECTS) $(PDDL_PARSER_OBJ) $(CFLAGS) $(LIBS)

# pddl syntax
scan-ops_pddl.tab.c: scan-ops_pddl.y lex.ops_pddl.c
	bison -pops_pddl -bscan-ops_pddl scan-ops_pddl.y

scan-fct_pddl.tab.c: scan-fct_pddl.y lex.fct_pddl.c
	bison -pfct_pddl -bscan-fct_pddl scan-fct_pddl.y

scan-mul_pddl.tab.c: scan-mul_pddl.y lex.mul_pddl.c
	bison -pmul_pddl -bscan-mul_pddl scan-mul_pddl.y

lex.fct_pddl.c: lex-fct_pddl.l
	flex -Pfct_pddl lex-fct_pddl.l

lex.ops_pddl.c: lex-ops_pddl.l
	flex -Pops_pddl lex-ops_pddl.l

lex.mul_pddl.c: lex-mul_pddl.l
	flex -Pmul_pddl lex-mul_pddl.l


# misc
clean:
	rm -f *.o *.bak *~ *% core *_pure_p9_c0_400.o.warnings \
        \#*\# $(RES_PARSER_SRC) $(PDDL_PARSER_SRC)

veryclean: clean
	rm -f fip H* J* K* L* O* graph.* *.symbex gmon.out \
	$(PDDL_PARSER_SRC) \
	lex.fct_pddl.c lex.ops_pddl.c lex.probname.c \
	*.output

depend:
	makedepend -- $(SOURCES) $(PDDL_PARSER_SRC)

lint:
	lclint -booltype Bool $(SOURCES) 2> output.lint

# DO NOT DELETE
