DIR = $(shell pwd)
ARCH = $(shell getconf LONG_BIT)
APP_NAME_64 = aeulnx
APP_NAME_32 = aeulnx32
APP_NAME = ${APP_NAME_${ARCH}}
INC_DIR = ${DIR}/platform/linux
SRC_DIR = ${DIR}
SRC2_DIR = ${DIR}/base
OUTDIR = ${DIR}/out/linux
SRC_OUTDIR = ${OUTDIR}/obj
SRC2_OUTDIR = ${SRC_OUTDIR}/base

INCS = constant.h io.h misc.h type.h
#SRCS = main.o ecgs.o ecge.o ecgf.o
SRCS = aec.o ecgs.o ecgf.o ecgx.o ecge.o ecgi.o ecgz.o ecg4.o
SRC2S = acpi.o util.o pmc.o rdc.o ite.o rdc4.o
INC2S = isp.h flash.h

HREADER =  ${INC2S:%=${SRC2_DIR}/%} ${INC_DIR}/*.h

CC = gcc
AR = ar
LFLAGS = -r
CFLAGS = -static -s -O2 -Wall -pthread
ENV = -D_LINUX_
#INC = -I ${SRC_DIR} ${SRC2_DIR} ${INC_DIR}
INC = -I ${DIR}
SRC_OBJS = ${SRCS:%=${SRC_OUTDIR}/%}
SRC2_OBJS = ${SRC2S:%=${SRC2_OUTDIR}/%}



COMPILER_32 = y
ifeq (${COMPILER_32}, y)

	SOURCE_SRC = aec.c ecgf.c ecgs.c ecgx.c ecge.c ecgi.c ecgz.c main.c ecg4.c

	SOURCE_BSE = ./base/acpi.c \
					./base/pmc.c \
					./base/rdc.c \
					./base/util.c \
					./base/ite.c \
					./base/rdc4.c
				
	SOURCE_PLF = ./platform/linux/misc.c
	
	SOURCE_ECT = ./customer/exfo_ec_update/som6867_exfo.c \
					./customer/ebc_kf08/ebc_kf08.c \
					./customer/epc_c301/epc_c301.c \
					./customer/mioe_260/mioe_260.c

	SOURCE_ALL = ${SOURCE_SRC} ${SOURCE_BSE} ${SOURCE_PLF} ${SOURCE_ECT}

	INC_SRC = aec.h ecgf.h ecgs.h ecgx.h ecge.h ecgi.h ecgz.h main.h ecg4.h

	INC_BSE = ./base/acpi.h \
				./base/pmc.h \
				./base/rdc.h \
				./base/util.h \
				./base/flash.h \
				./base/isp.h \
				./base/ite.h \
				./base/rdc4.h
				
	INC_PLF = ./platform/adaptate.h \
				./platform/linux/constant.h \
				./platform/linux/io.h \
				./platform/linux/misc.h \
				./platform/linux/type.h
				
	INC_ECT = ./customer/exfo_ec_update/som6867_exfo.h \
				./customer/ebc_kf08/ebc_kf08.h \
				./customer/epc_c301/epc_c301.h \
				./customer/mioe_260/mioe_260.h
	
	INC_ALL = ${INC_SRC} ${INC_BSE} ${INC_PLF} ${INC_ECT}

aeulnx:
	@mkdir -p ${OUTDIR}
	
	@echo "Build aeulnx..."
	@${CC} ${CFLAGS} ${INC} ${ENV} -o ${OUTDIR}/${APP_NAME_64} ${INC_ALL} ${SOURCE_ALL}
	@echo "Build aeulnx success..."
	
	@echo "Build aeulnx32..."
	@${CC} ${CFLAGS} ${INC} ${ENV} -m32 -o ${OUTDIR}/${APP_NAME_32} ${INC_ALL} ${SOURCE_ALL}
	@echo "Build aeulnx32 success..."

else

#aeulnx: setup ${SRC2_OBJS} ${SRC_OBJS} ${SRC_OUTDIR}/misc.o ${INC_DIR}/../adaptate.h 
aeulnx: setup main.c main.h ${SRC2_OBJS} ${SRC_OBJS} ${SRC_OUTDIR}/misc.o ${INC_DIR}/../adaptate.h 
	@${CC} main.c ${SRC_OBJS} ${SRC2_OBJS}  ${SRC_OUTDIR}/misc.o ${CFLAGS} ${INC} ${ENV} -o ${OUTDIR}/${APP_NAME}
	@echo "SUCCESS..."

endif



setup:
	@mkdir -p ${SRC_OUTDIR}
	@mkdir -p ${SRC2_OUTDIR}

${SRC2_OUTDIR}/%.o : ${SRC2_DIR}/%.c ${SRC2_DIR}/%.h ${HREADER} ${SRC_OUTDIR}/misc.o ${INC_DIR}/../adaptate.h 
	@echo "build "$<
	@${CC} $< ${CFLAGS} ${INC} ${ENV} -c -o $@

${SRC_OUTDIR}/%.o : ${SRC_DIR}/%.c ${SRC_DIR}/%.h ${HREADER} ${SRC2_OBJS} ${SRC_OUTDIR}/misc.o ${INC_DIR}/../adaptate.h 
	@echo "build "$<
	@${CC} $< -l ${CFLAGS} ${INC} ${ENV} -c -o $@
	
${SRC_OUTDIR}/misc.o : ${INC_DIR}/misc.c ${INC_DIR}/misc.h ${INC_DIR}/../adaptate.h 
	@echo "build "$<
	@${CC} $< ${CFLAGS} ${INC} ${ENV} -c -o $@

.PHONY: clean all
all : clean setup aeulnx

clean: 
	@rm -rf ${SRC_OUTDIR}/*


