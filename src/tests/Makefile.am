## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

AM_CFLAGS =\
	 -Wall\
	 -g\
	 -O0\
	 -I$(srcdir)/..

bin_PROGRAMS = 2048_test

2048_test_SOURCES =  \
	test.c

2048_test_LDFLAGS =

2048_test_LDADD = ../ai/utils/list.o ../ai/utils/tree.o ../ai/evaluator.o \
../models/board.o ../views/output.o ../ai/expectimax.o ../ai/utils/hash.o \
../ai/board_pool.o ../models/calculator.o
