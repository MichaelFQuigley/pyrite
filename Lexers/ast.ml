type atom = 
    | LIT of string
    | VARIABLE of string
    | FCALL of string * expr_stmt array
    | PAREN_EXPR of expr_stmt
and expr_stmt = 
    | BINOP of string * expr_stmt * expr_stmt
    | IF of expr_stmt * expr_stmt * expr_stmt
    | ATOMOP of atom;;
type simple_stmt = 
    | EXPROP of expr_stmt;;
type stmts = 
    | STMTS of simple_stmt array;;



let rec string_of_stmts ast = 
    let rec handle_arr args = 
        ("StmtOp( "^(string_of_simple_stmt (Array.get args 0))^" )\n"^(if ((Array.length args) > 1) then handle_arr (Array.sub args 1 ((Array.length args) - 1))
        else ""))
    in
    match ast with
    |  STMTS args -> handle_arr args
and string_of_simple_stmt ast = 
    match ast with
    | EXPROP op -> 
            ("ExprOp( "^(string_of_expr_stmt op)^" )")
and string_of_expr_stmt ast = 
    match ast with
    | BINOP (op, expa, expb) -> ("BinOp( "^op^", "^(string_of_expr_stmt expa)^", "^(string_of_expr_stmt expb)^" )")
    | ATOMOP at -> ("AtomOp("^(string_of_atom at)^")")
    | IF (a,b,c) -> ("IfOp( "^(string_of_expr_stmt a)^", "^(string_of_expr_stmt b)^", "^(string_of_expr_stmt c))^" )"
and string_of_atom ast = 
    match ast with
    | LIT a -> ("LIT("^a^")")
    | VARIABLE a -> ("Variable("^a^")")
    | FCALL (name,exprs) -> ("Fcall("^name^" ("^(args_printer exprs)^"))")
    | _ -> raise (Failure "Invalid ast.")
    and args_printer args = 
        if (Array.length args) == 0 then ""
        else (", "^(string_of_expr_stmt (Array.get args 0))^(if ((Array.length args) > 1) then args_printer (Array.sub args 1 ((Array.length args) - 1))
                                                                           else ""));;




