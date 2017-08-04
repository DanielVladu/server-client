#! /bin/bash
var=1
for i in {1..50}; do
	var="$(( $i * 10 ))"
	head -c ${var}M </dev/urandom > fileR${var}
done
