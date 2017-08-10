CC := g++

ROOT_DIR := ..
SRC_DIR := ${ROOT_DIR}/src
UTILS_DIR := ${ROOT_DIR}/utils
INC_DIR := ${ROOT_DIR}/include
LIB_DIR := ${ROOT_DIR}/lib

CPP_SRC := ${shell find ${SRC_DIR} -name "*.cpp"}
CPP_SRC += ${shell find ${UTILS_DIR} -name "*.cpp"}
H_SRC := ${shell find ${SRC_DIR} -name "*.h"}
H_SRC += ${shell find ${UTILS_DIR} -name "*.h"}
H_SRC += ${shell find ${INC_DIR} -name "*.h"}

H_DIR := ${addprefix -I, ${dir ${H_SRC}}}
OBJECTS := ${patsubst %.cpp, %.o, ${CPP_SRC}}

CFLAGS := -g -Wall -lpthread
CFLAGS += -DDEBUG

TARGET := safety

all : clean ${TARGET}

${TARGET} : ${OBJECTS}
	${CC} ${CFLAGS} ${H_DIR} $^ -o $@

${OBJECTS} : ${CPP_SRC}
	${CC} -c ${CFLAGS} ${H_DIR} ${patsubst %.o, %.cpp, $@} -o $@

.PHNOY: clean
clean:
	-rm -rf ${TARGET} ${OBJECTS}
