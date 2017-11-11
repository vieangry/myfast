## Process this file with automake to produce Makefile.in


bin_PROGRAMS =  \
	xofp

xofp_SOURCES =  \
	main_xofp.c

xofp_CFLAGS = -L../librule \
	-L../libofp \
	-L../libreg \
	-I \
	../include 

xofp_LDADD = -lofp \
	-lrule \
	-lreg \
	-lpthread \
	-lpcap \
	-lnet 