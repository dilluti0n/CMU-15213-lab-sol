#!/bin/sh

docker run --rm -v /Users/hskimse/Desktop/prjt/CMU-15213-lab-sol/6_MallocLab/malloclab-handout:/app ubuntu32 sh -c "cd /app && make mdriver && ./mdriver -t ./traces/ -V -v"
