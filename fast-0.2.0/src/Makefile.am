## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = fast

fast_SOURCES = \
	main.c

fast_CFLAGS = -L../librule \
	-L../libua \
	-L../libreg \
	-I \
	../include 

fast_LDADD =  \
	-lrule \
	-lua \
	-lreg \
	-lpthread

