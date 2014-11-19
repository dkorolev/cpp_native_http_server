#!/bin/bash
#
# curl with response code printing. Surpasses progress and loads localhost:8080 by default.

CMD="curl -s -w \n%{http_code}\n"

if [ $# -gt 0 ]
	then $CMD $*
	else $CMD localhost:8080
fi
