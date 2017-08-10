#!/bin/bash

for i in {1..50}; do
	 var="$(( $i * 1))"
	 ./sock_client.o 127.0.0.1 1225 small_test_files/fileR${var}
done 

