#ocamlc -pp "camlp4o pa_extend.cmo" -I +camlp4  token.ml lexer.ml
mkdir -p build
ocamlc -pp "camlp4o pa_extend.cmo" -I +camlp4  util.ml token.ml lexer.ml ast.ml parser.ml main.ml -o "build/parse.out"
rm *.cmi
rm *.cmo
