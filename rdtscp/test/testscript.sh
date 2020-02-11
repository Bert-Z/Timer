#!/bin/bash

g++ -std=c++11 ../timer.h ../main.cpp -o rdtscp

for ((i=0; i<100; i++))
do
	./rdtscp >> out.o
done