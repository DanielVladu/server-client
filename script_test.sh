#!/bin/bash

for i in {1..50}; do
	 var="$(( $i * 10))"
	 ./sock_client.o 127.0.0.1 1223 test_files/fileR${var}
done 

