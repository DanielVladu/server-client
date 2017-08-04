#!/bin/bash
for i in {1..50}; do
	var="$(( $i * 1 ))"
	diff fileR${var} ../received_fileR${var}
done
