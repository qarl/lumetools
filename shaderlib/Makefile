SHELL=/bin/sh

FLEXVER = v5.12
FLEXINC = /project/shader/src/flexlm/$(FLEXVER)/machind
FLEXLIB = /project/shader/src/flexlm/$(FLEXVER)/sgi_u5/liblmgr.a

CFLAGS = -O -I/pkg/softimage_3.7/mental_ray/inc -I$(FLEXINC)

OFILES = calc.o gamma.o color.o lume_state.o random.o \
	frac.o lic_int.o

all: liblume.a lic_make


liblume.a: $(OFILES) $(FLEXLIB)
	cp $(FLEXLIB) liblume.a
	ar r liblume.a $(OFILES)


clean:
	rm -f *~ *.o *.a a.out



lic_test: lic_test.c liblume.a
	cc -o lic_test lic_test.c liblume.a

lic_make: lic_make.c liblume.a
	cc $(CFLAGS) -o lic_make lic_make.c liblume.a

