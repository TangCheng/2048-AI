## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

AM_CFLAGS =\
	 -Wall\
	 -g\
	 -O2

AM_CXXFLAGS = \
	-Wall\
	-g

bin_PROGRAMS = 2048

2048_SOURCES = \
	views/output.c \
	models/board.c \
	models/random.c \
	models/game.c \
	models/calculator.c \
	controllers/input.c \
	ai/ai.c \
	ai/evaluator.c \
	ai/minmax.c \
	ai/tree.c \
	ai/list.c \
	ai/board_pool.c \
	ai/expectimax.c \
	main.c

2048_LDFLAGS =

2048_LDADD =
