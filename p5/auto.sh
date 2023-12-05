#!/bin/bash

# Loop from 1 to 4
for i in {1..4}
do
    ./s$i < s$i.solution
done

