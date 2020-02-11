#!/bin/bash

g++ ../clock_gettime.cpp -o clock_gettime

for ((i=0; i<100; i++))
do
	./clock_gettime >> out.o
done