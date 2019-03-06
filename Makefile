SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=mpicc
CGPU=nvcc
CFLAGS=-O3 -I$(HEADER_DIR)
GPUFLAGS= -I$(HEADER_DIR)
LDFLAGS=-lm -L/usr/local/cuda/lib64 -lcudart

SRC= dgif_lib.c \
	egif_lib.c \
	gif_err.c \
	gif_font.c \
	gif_hash.c \
	gifalloc.c \
	gpu_filters.cu \
	mpi_filters.c \
	omp_filters.c \
	seq_functions.c \
	main.c \
	openbsd-reallocarray.c \
	quantize.c

OBJ= $(OBJ_DIR)/dgif_lib.o \
	$(OBJ_DIR)/egif_lib.o \
	$(OBJ_DIR)/gif_err.o \
	$(OBJ_DIR)/gif_font.o \
	$(OBJ_DIR)/gif_hash.o \
	$(OBJ_DIR)/gifalloc.o \
	$(OBJ_DIR)/gpu_filters.o \
	$(OBJ_DIR)/omp_filters.o \
	$(OBJ_DIR)/mpi_filters.o \
	$(OBJ_DIR)/seq_functions.o \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/openbsd-reallocarray.o \
	$(OBJ_DIR)/quantize.o

all: $(OBJ_DIR) sobelf

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -fopenmp $(CFLAGS) -c -o $@ $^

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cu
	$(CGPU) $(GPUFLAGS) -c -o $@ $^

sobelf:$(OBJ)
	$(CC) -fopenmp $(CFLAGS)  $(LDFLAGS) -o $@ $^

clean:
	rm -f sobelf $(OBJ)
