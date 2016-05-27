let rec string_of_stmt ast = 
    match ast with
    | Ast.EXPROP op -> 
            ("ExprOp( "^(string_of_expr_stmt op)^" )\n")
    | _ -> raise (Failure "Invalid ast.")
and string_of_expr_stmt ast = 
    match ast with
    | Ast.BINOP (op, expa, expb) -> ("BinOp( "^op^", "^(string_of_expr_stmt expa)^", "^(string_of_expr_stmt expb)^" )")
    | Ast.ATOMOP at -> ("AtomOp("^(string_of_atom at)^")")
    | _ -> raise (Failure "Invalid ast.")
and string_of_atom ast = 
    match ast with
    | Ast.LIT a -> ("LIT("^a^")")
    | Ast.VARIABLE a -> ("Variable("^a^")")
    | Ast.FCALL (name,exprs) -> ("Fcall("^name^" ("^(args_printer exprs)^"))")
    | _ -> raise (Failure "Invalid ast.")
    and args_printer args = 
        if (Array.length args) == 0 then ""
        else (", "^(string_of_expr_stmt (Array.get args 0))^(if ((Array.length args) > 1) then args_printer (Array.sub args 1 ((Array.length args) - 1))
                                                                           else ""));;



let main = 
    Hashtbl.add Parser.op_precedence "+" 20;
    Hashtbl.add Parser.op_precedence "-" 20;
    Hashtbl.add Parser.op_precedence "*" 30;
    Hashtbl.add Parser.op_precedence "/" 30;
    let in_file = open_in "testFile.my" in
    let token_stream = Lexer.lex (Stream.of_channel in_file) in
    (*print_string (Token.string_of_tok_stream token_stream)*)
    print_string (string_of_stmt (Parser.parse_top token_stream))
;;



main
