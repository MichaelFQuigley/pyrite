type atom = 
    | LIT of string
    | VARIABLE of string
    | FCALL of string * expr_stmt array
    | PAREN_EXPR of expr_stmt
and expr_stmt = 
    | BINOP of string * expr_stmt * expr_stmt
    | IF of expr_stmt array
    | LOOP of expr_stmt * expr_stmt * expr_stmt * expr_stmt
    | ATOMOP of atom;;
type simple_stmt = 
    | EXPROP of expr_stmt
type stmts = 
    | STMTS of simple_stmt array;;



let rec string_of_stmts ast = 
    let rec handle_arr args = 
        (make_json_kv "StmtOp" (string_of_simple_stmt (Array.get args 0)))
        ^(if ((Array.length args) > 1) then 
            (", "^handle_arr (Array.sub args 1 ((Array.length args) - 1)))
            else "")
    in
    match ast with
    |  STMTS args -> make_json_kv "StmtsOp" ("["^(handle_arr args)^"]")
and string_of_simple_stmt ast = 
    match ast with
    | EXPROP op -> 
            make_json_kv "ExprOp" (string_of_expr_stmt op)
and string_of_expr_stmt ast = 
    match ast with
    | BINOP (op, expa, expb) -> 
            make_json_kv "BinOp" ("["^op^", "^(string_of_expr_stmt expa)^", "^(string_of_expr_stmt expb)^"]")
    | ATOMOP at -> 
            make_json_kv "AtomOp" (string_of_atom at)
    | IF args -> 
            make_json_kv "IfOp" ("["^(args_printer args)^"]")
    | LOOP (a,b,c,d) -> 
            make_json_kv "LoopOp" ("["^(string_of_expr_stmt a)^", "^(string_of_expr_stmt b)^", "^(string_of_expr_stmt c)^", "^(string_of_expr_stmt d)^"]")
and string_of_atom ast = 
    match ast with
    | LIT a -> 
            make_json_kv "Lit" a
    | VARIABLE a -> 
            make_json_kv "Variable" ("\""^a^"\"")
    | FCALL (name,exprs) -> 
            make_json_kv "Fcall" ("["^name^", "^(args_printer exprs)^"]")
    | _ -> raise (Failure "Invalid ast.")
    and args_printer args = 
        if (Array.length args) == 0 then ""
        else ((string_of_expr_stmt (Array.get args 0))^(if ((Array.length args) > 1) then ", "^(args_printer (Array.sub args 1 ((Array.length args) - 1)))
                                                                           else ""))
and make_json_kv key value = 
    "{\""^key^"\": "^value^"}";;




