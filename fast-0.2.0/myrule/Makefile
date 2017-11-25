
demo1_LDADD = -L ../include -L ../libreg\
	-L../librule \
	-L../libua \
	-lrule \
	-lua \
	-lreg \
	-lpthread
edit:printrule.o
	cc -o edit printrule.o $(demo1_LDADD)

printrule.o:printrule.c
	cc -c printrule.c
clean:
	rm edit printrule.o
