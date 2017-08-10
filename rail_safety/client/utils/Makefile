CC := g++

ROOT_DIR := ..
SRC_DIR := ${ROOT_DIR}/utils
INC_DIR := ${ROOT_DIR}/include

C_SRC := ${shell find ${SRC_DIR} -name "*.cpp"}
H_SRC := ${shell find ${SRC_DIR} -name "*.h"}
H_SRC += ${shell find ${INC_DIR} -name "*.h"}

C_DIR := ${dir ${C_SRC}}
H_DIR := ${addprefix -I, ${dir ${H_SRC}}}

OBJECTS := ${patsubst %.cpp, %.o, ${C_SRC}}

CFLAGS := -g -Wall
CFLAGS += -DUTILS_TEST
TARGET := a.out 

all : clean ${TARGET}

${TARGET} : ${OBJECTS}
	${CC} ${CFLAGS} ${H_DIR} $^ -o $@

${OBJECTS} : ${C_SRC}
	${CC} -c ${CFLAGS} ${H_DIR} ${patsubst %.o, %.cpp, $@} -o $@

.PHNOY: clean
clean:
	-rm -rf ${TARGET} ${OBJECTS}
