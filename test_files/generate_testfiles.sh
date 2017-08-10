#! /bin/bash
var=1
for i in {1..25}; do
	var="$(( $i * 20 ))"
	head -c ${var}M </dev/urandom > fileR${var}
done
