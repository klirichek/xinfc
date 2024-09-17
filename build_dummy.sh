#!/bin/bash
gcc -g -O0 -Wall -Iinclude src/xinfc-wsc.c -DXINFC_DUMMY_OUT -o xinfc-wsc
