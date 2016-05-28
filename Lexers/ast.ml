type atom = 
    | LIT of string
    | VARIABLE of string
    | FCALL of string * expr_stmt array
    | PAREN_EXPR of expr_stmt
and expr_stmt = 
    | BINOP of string * expr_stmt * expr_stmt
    (*format of if: test_expr * array(stmts_true, [stmts_false])*)
    | IF of expr_stmt * stmts array
    | LOOP of expr_stmt array * stmts
    | ATOMOP of atom
and simple_stmt = 
    | EXPROP of expr_stmt
and stmts = 
    | STMTS of simple_stmt array;;


let rec args_printer args func comma_at_end= 
    if (Array.length args) == 0 then ""
    else ((func (Array.get args 0))^(if ((Array.length args) > 1) then ", "^(args_printer (Array.sub args 1 ((Array.length args) - 1)) func comma_at_end)
        else (if comma_at_end then ", " else "")));;
 
let rec string_of_stmts ast = 
    let rec handle_arr args = 
        (string_of_simple_stmt (Array.get args 0))
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
            make_json_kv "BinOp" ("[\""^op^"\", "^(string_of_expr_stmt expa)^", "^(string_of_expr_stmt expb)^"]")
    | ATOMOP at -> 
            make_json_kv "AtomOp" (string_of_atom at)
    | LOOP (exprs_arr, stmts) -> 
            make_json_kv "LoopOp" ("["^(args_printer exprs_arr string_of_expr_stmt true)^(string_of_stmts stmts)^"]")
    | IF (test_expr, stmts_arr) -> 
            make_json_kv "IfOp" ("["^(string_of_expr_stmt test_expr)^", "^(args_printer stmts_arr string_of_stmts false)^"]")
and string_of_atom ast = 
    match ast with
    | LIT a -> 
            make_json_kv "Lit" a
    | VARIABLE a -> 
            make_json_kv "Variable" ("\""^a^"\"")
    | FCALL (name,exprs) -> 
            make_json_kv "Fcall" ("[\""^name^"\", "^(args_printer exprs string_of_expr_stmt false)^"]")
    | _ -> raise (Failure "Invalid ast.")
and make_json_kv key value = 
    "{\""^key^"\": "^value^"}";;




