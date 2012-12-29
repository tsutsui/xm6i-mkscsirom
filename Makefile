CC=		cc
COPTS=		-O
CFLAGS=		${COPTS}
CFLAGS+=	-Wall
SRC=		mkscsirom.c

TARGETS=	mkscsiin mkscsiex mkrom30

all:	${TARGETS}

mkscsiin: ${SRC}
	${CC} ${CFLAGS} -DSCSIIN ${SRC} -o ${.TARGET}

mkscsiex: ${SRC}
	${CC} ${CFLAGS} -DSCSIEX ${SRC} -o ${.TARGET}

mkrom30: ${SRC}
	${CC} ${CFLAGS} -DROM30  ${SRC} -o ${.TARGET}

clean:
	rm -f *.o ${TARGETS}
