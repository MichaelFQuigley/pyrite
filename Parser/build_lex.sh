#ocamlc -pp "camlp4o pa_extend.cmo" -I +camlp4  token.ml lexer.ml
ocamlc -pp "camlp4o pa_extend.cmo" -I +camlp4  util.ml token.ml lexer.ml ast.ml parser.ml main.ml -o "build/parse.out"
