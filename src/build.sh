#!/bin/bash
set -e
set +x

ocamlfind ocamlc -g -o main -linkpkg \
	-package xmlm -package zip -package extlib \
	prelude.ml acic.ml interpreter.ml counter.ml parser.ml main.ml
