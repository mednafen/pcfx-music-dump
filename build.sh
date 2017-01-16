#!/bin/sh

g++ -Wall -O2 -o dlfx dlfx.cpp -lsndfile && \
g++ -Wall -O2 -o farland farland.cpp -lsndfile && \
g++ -Wall -O2 -o zeroigar zeroigar.cpp -lsndfile &&
echo "Done.\n"

