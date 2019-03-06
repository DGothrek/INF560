
all:bandwidthTest deviceQuery

bandwidthTest:bandwidthTest.cu
	nvcc -I. -o bandwidthTest bandwidthTest.cu

deviceQuery: deviceQuery.cpp
	nvcc -I. -o deviceQuery deviceQuery.cpp 


clean:
	rm -f bandwidthTest deviceQuery
