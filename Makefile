.POSIX:

#
# COMPILER FLAGS ( stored in config.mk file )
#

include config.mk

#
# SRC
#

SRC=main.c

#
# OBJS
#

OBJ=main.o

#
# TARGET
#

TARGET=program

.PHONY: test clean

.SILENT: test

.IGNORE: clean


${TARGET}: ${OBJ}
	${CC} ${SRC} -o ${TARGET} ${CFLAGS} ${INC} ${LIBS}

.SUFFIXES: .c .o

.c.o:
	${CC} -c ${CFLAGS} $< -o $@ ${INC}

test: ${TARGET}
	-./${TARGET}

.SILENT: clean

clean:
	rm -vf ${TARGET} ${OBJ}
