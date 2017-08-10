#!/bin/bash
for i in {1..25}; do
	var="$(( $i * 20 ))"
	diff fileR${var} ../received_fileR${var}
done
