#!/bin/bash
set -e
set +x

ocamlfind ocamlc -g -o parser -linkpkg -package xmlm prelude.ml acic.ml parser.ml
