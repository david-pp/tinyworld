#!/bin/sh

for ((i = 1; i < 4 ; ++ i)) do
	./client $i &
done
