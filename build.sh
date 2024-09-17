#!/bin/bash
C=${C:-gcc}
$C -O2 -Wall -Iinclude src/xinfc-wsc.c -o xinfc-wsc
