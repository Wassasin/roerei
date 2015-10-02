#!/bin/bash
set -e
set +x

ocamlfind ocamlc -g -o main -linkpkg -syntax camlp4o \
	-package str -package xmlm -package zip -package extlib \
	-package msgpack -package msgpack.conv -package meta_conv.syntax \
	prelude.ml acic.ml object.ml interpreter.ml counter.ml \
	parser.ml main.ml
