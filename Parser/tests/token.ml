type token = 
    | LPAREN
    | RPAREN
    | LBRAC
    | RBRAC
    | LSQ
    | RSQ
    | LIT of string
    | PUNCT of string
    | IDENT of string
    | KWD of string
    | EOF

let rec string_of_tok_stream str = 
    match str with parser
    | [<'LPAREN; stream>] -> ("(LPAREN)\n")^(string_of_tok_stream str)
    | [<'RPAREN; stream>] -> ("(RPAREN)\n")^(string_of_tok_stream str)
    | [<'LBRAC; stream>] -> ("(LBRAC)\n")^(string_of_tok_stream str)
    | [<'RBRAC; stream>] -> ("(RBRAC)\n")^(string_of_tok_stream str)
    | [<'LSQ; stream>] -> ("(LSQ)\n")^(string_of_tok_stream str)
    | [<'RSQ; stream>] -> ("(RSQ)\n")^(string_of_tok_stream str)
    | [<'LIT arg ; stream>] -> "(LIT "^arg^")\n"^(string_of_tok_stream str)
    | [<'PUNCT arg ; stream>] -> "(PUNCT "^arg^")\n"^(string_of_tok_stream str)
    | [<'IDENT arg ; stream>] -> "(IDENT "^arg^")\n"^(string_of_tok_stream str)
    | [<'KWD arg ; stream>] -> "(KWD "^arg^")\n"^(string_of_tok_stream str)
    | [<'EOF; stream>] -> ("(EOF)\n")^(string_of_tok_stream str)
    | [< >] -> "etet\n";;
  
