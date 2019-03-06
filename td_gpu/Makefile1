all: hello_cpu add_vectors main

main: main.c
	gcc -o main main.c

hello_cpu: hello_cpu.cu
	nvcc -I. -o hello_cpu hello_cpu.cu

add_vectors: add_vectors.cu
	nvcc -I. -o add_vectors add_vectors.cu

clean:
	rm -f hello_cpu add_vectors
 
