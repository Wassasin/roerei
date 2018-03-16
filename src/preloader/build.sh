#!/bin/bash
set -e
set +x

ocamlfind ocamlc -g -o preloader -linkpkg -syntax camlp4o \
	-package str -package xmlm -package zip -package extlib \
	-package msgpack -package msgpack.conv -package meta_conv.syntax \
	-package tiny_json -package tiny_json_conv \
	-package treeprint \
	prelude.ml acic.ml counter.ml util.ml object.ml storage.ml \
	interpreter.ml acicparser.ml minlister.ml preloader.ml
