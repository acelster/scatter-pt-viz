# Copyright (c) 2013, Thomas L. Falch
# For conditions of distribution and use, see the accompanying LICENSE and README files

# This file is a part of the Scattered Point Visualization application
# developed at the Norwegian University of Science and Technology

float=true

SHELL=/bin/bash
CC=gcc
CFLAGS=-std=c99 -O3 -g -I/usr/local/cuda/include -fopenmp
NVFLAGS= -arch=compute_30 -code=sm_30 --ptxas-options=-v -Xcompiler -fopenmp
LDFLAGS= -L/usr/local/cuda/lib64 -L/usr/lib/nvidia-current -L/usr/lib/nvidia-304 -lrt -lm -lGL -lGLU -lglut -pthread -lhdf5 -llapack -lblas -lcuda -lcudart -fopenmp -g

SOURCES_ALL=$(wildcard *.c)
SOURCES=$(filter-out main.c perf.c simple.c test.c, $(SOURCES_ALL))
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
CUDA_VERSION_OBJECTS=$(patsubst %.c,%.cuda.o,$(SOURCES))
CUDA_SOURCES=$(wildcard *.cu)
CUDA_OBJECTS=$(patsubst %.cu, %.o, $(CUDA_SOURCES))

ifeq ($(float),true)
CFLAGS += -DUSE_FLOAT
NVFLAGS += -DUSE_FLOAT
endif

all : cuda_spv spv

spv : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) main.c $(LDFLAGS) -o spv.out
	
cuda_spv : $(CUDA_VERSION_OBJECTS) $(CUDA_OBJECTS)
	$(CC) $(CFLAGS) -DUSE_CUDA $(CUDA_VERSION_OBJECTS) $(CUDA_OBJECTS) main.c $(LDFLAGS) -o cuda_spv.out
	
$(CUDA_OBJECTS): %.o : %.cu
	nvcc $(NVFLAGS) $< -c -o $@
	
$(CUDA_VERSION_OBJECTS): %.cuda.o : %.c
	$(CC) $(CFLAGS) -DUSE_CUDA -c -o $@ $<
	
clean:
	rm -f *.o spv.out cuda_spv.out
