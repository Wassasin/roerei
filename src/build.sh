#!/bin/bash
set -e
set +x

ocamlfind ocamlc -g -o preloader -linkpkg -syntax camlp4o \
	-package str -package xmlm -package zip -package extlib \
	-package msgpack -package msgpack.conv -package meta_conv.syntax \
	prelude.ml acic.ml counter.ml util.ml object.ml interpreter.ml \
	parser.ml preloader.ml

ocamlfind ocamlc -g -o tester -linkpkg -syntax camlp4o \
	-package str \
	-package msgpack -package msgpack.conv -package meta_conv.syntax \
	prelude.ml acic.ml counter.ml util.ml object.ml tester.ml
