kentSrc = ../../..
include ../../../inc/common.mk


L += $(MYSQLLIBS) -lm
MYLIBDIR = ../../../lib/$(MACHTYPE)
MYLIBS =  $(MYLIBDIR)/jkhgap.a $(MYLIBDIR)/jkweb.a

O = checkPsl.o 

checkPsl: $O $(MYLIBS)
	${CC} ${COPT} ${CFLAGS} -o checkPsl $O $(MYLIBS) $L

lib:
	cd ../../lib && ${MAKE}
	cd ../../../lib && ${MAKE}

clean::
	rm -f ${O}
