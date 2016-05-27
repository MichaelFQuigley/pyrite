let main = 
    Hashtbl.add Parser.op_precedence "+" 20;
    Hashtbl.add Parser.op_precedence "-" 20;
    Hashtbl.add Parser.op_precedence "*" 30;
    Hashtbl.add Parser.op_precedence "/" 30;
    let in_file = open_in "testFile.my" in
    let token_stream = Lexer.lex (Stream.of_channel in_file) in
    Parser.parse_top token_stream
;;
main
