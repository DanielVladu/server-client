#!/bin/bash

for i in {1..25}; do
	 var="$(( $i * 20))"
	 ./sock_client.o 127.0.0.1 1225 test_files/fileR${var}
done 

