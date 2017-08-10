#!/bin/bash

dmesg > foo.txt
for i in {1..1000}; do
	dmesg >> foo.txt
done
