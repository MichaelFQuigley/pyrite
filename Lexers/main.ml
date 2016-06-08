let main = 
    Hashtbl.add Parser.op_precedence "+" 20;
    Hashtbl.add Parser.op_precedence "-" 20;
    Hashtbl.add Parser.op_precedence "*" 30;
    Hashtbl.add Parser.op_precedence "/" 30;
    Hashtbl.add Parser.op_precedence "%" 30;
    Hashtbl.add Parser.op_precedence "|" 7;
    Hashtbl.add Parser.op_precedence "&" 8;
    Hashtbl.add Parser.op_precedence "^" 9;
    Hashtbl.add Parser.op_precedence ">" 10;
    Hashtbl.add Parser.op_precedence "<" 10;
    Hashtbl.add Parser.op_precedence "<=" 10;
    Hashtbl.add Parser.op_precedence ">=" 10;
    Hashtbl.add Parser.op_precedence "==" 10;
    Hashtbl.add Parser.op_precedence "!=" 10;
    Hashtbl.add Parser.op_precedence "=" 5;
    let in_file = open_in "testFile.my" in
    let token_stream = Lexer.lex (Stream.of_channel in_file) in
    (*print_string (Token.string_of_tok_stream token_stream)*)
    print_string (Ast.string_of_stmts (Parser.parse_top token_stream))
;;



main
