#!/bin/sh
nanopb_generator.py -L "#include <nanopb/%s>" -I . -D . high_to_low.proto low_to_high.proto
