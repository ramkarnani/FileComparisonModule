OMP: driverOMP.cpp
	g++ -fopenmp -o showOMP driverOMP.cpp -I/usr/include/hashlib++/ -lhl++
pthread: driverPthread.cpp
	g++ -pthread -o showPthread driverPthread.cpp -I/usr/include/hashlib++/ -lhl++
