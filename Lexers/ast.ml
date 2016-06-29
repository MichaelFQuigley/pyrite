type atom = 
    | LIT of string
    | RANGE of atom * atom
    | FCALL of string * expr_stmt array
    | PAREN_EXPR of expr_stmt
    | VARIABLE of string
and expr_stmt = 
    | BINOP of string * expr_stmt * expr_stmt
    (*format of if: test_expr * array(stmts_true, [stmts_false])*)
    | IF of expr_stmt * stmts array
    (*format of for loop, index_var * iterator * stmts*)
    | ATOMOP of atom
    | FUNCDEF of string * func_proto * stmts
    | LIST of expr_stmt array
and simple_stmt = 
    | EXPROP of expr_stmt
    (*format of VARDEF is variable * expression assigned to variable*)
    | VARDEF of string * expr_stmt
    | RETURN of stmts
    | FOR of atom * atom * stmts
    | WHILE of expr_stmt * stmts
and func_proto = 
    (*format of FUNC_PROTO is array(arg1, arg2, ...)*)
    | FUNC_PROTO of string array
and stmts = 
    | STMTS of simple_stmt array

let rec args_printer args func comma_at_end= 
    if (Array.length args) == 0 then ""
    else ((func (Array.get args 0))^(if ((Array.length args) > 1) then 
                                       ", "^(args_printer (Array.sub args 1 ((Array.length args) - 1)) func comma_at_end)
        else (if comma_at_end then ", " else "")));;

(*make_json_arr creates a string representing a json array.
 * arr is the array of functions, and func is a function that specifies
 * how to stringify each element in the array.*)
let rec make_json_arr arr func = 
    "["^(args_printer arr func false)^"]"
and make_json_kv key value = 
    make_json_kvs [key] [value]
(*make_json_kvs creates a string representing a json dict
 * both 'keys' and 'values' that are passed in are lists.*)
and make_json_kvs keys values = 
    let rec json_kvs_helper keys values =
        ("\""^(List.hd keys)^"\": "^(List.hd values)^
        (if (List.length keys > 1) then
            (", "^(json_kvs_helper (List.tl keys) (List.tl values)))
        else ""))
    in ("{"^(json_kvs_helper keys values)^"}");;

let rec string_of_stmts ast = 
    match ast with
    |  STMTS args -> make_json_kv "StmtsOp" (make_json_arr args string_of_simple_stmt)
and string_of_prototype ast = 
    match ast with
    | FUNC_PROTO args_arr -> make_json_kv "FuncProto" (make_json_kv "args" 
                                                                     (make_json_arr args_arr (fun n -> ("\""^n^"\""))))

and string_of_simple_stmt ast = 
    match ast with
    | EXPROP op -> 
            make_json_kv "ExprOp" (string_of_expr_stmt op)
    | VARDEF (v, expr) ->
            make_json_kv "VarDef" (make_json_kvs ["var"; "expr"]
                                                 ["\""^v^"\""; string_of_expr_stmt expr])
    | RETURN s -> 
            make_json_kv "ReturnOp" (string_of_stmts s)
    | FOR (loop_var, itt, stmts) -> 
            make_json_kv "ForOp" (make_json_kvs ["loop_var"; "itt" ; "body"] 
            [(string_of_atom loop_var); (string_of_atom itt); (string_of_stmts stmts)])
    | WHILE (header, stmts) ->
            make_json_kv "WhileOp" (make_json_kvs ["header"; "body"]
            [string_of_expr_stmt header; string_of_stmts stmts])
and string_of_expr_stmt ast = 
    match ast with
    | BINOP (op, expa, expb) -> 
            make_json_kv "BinOp" (make_json_kvs ["op"; "lhs"; "rhs"] 
                                                ["\""^op^"\""; (string_of_expr_stmt expa); (string_of_expr_stmt expb)])
    | ATOMOP at -> 
            make_json_kv "AtomOp" (string_of_atom at)
    | IF (test_expr, stmts_arr) -> 
            make_json_kv "IfOp" (make_json_kvs ["test"; "bodies"]
                                               [(string_of_expr_stmt test_expr); make_json_arr stmts_arr string_of_stmts])
    | FUNCDEF (name, proto, stmts) ->
            make_json_kv "FuncDef" (make_json_kvs ["name"; "header"; "stmts"]
                                ["\""^name^"\""; string_of_prototype proto; string_of_stmts stmts])
    | LIST elements ->
            make_json_kv "ListOp" (make_json_arr elements string_of_expr_stmt)
and string_of_atom ast = 
    match ast with
    | LIT a -> 
            make_json_kv "Lit" a
    | VARIABLE a -> 
            make_json_kv "Variable" ("\""^a^"\"")
    | FCALL (name,exprs_arr) -> 
            make_json_kv "FCall" (make_json_kvs ["name"; "args"]
                                                ["\""^name^"\""; make_json_arr exprs_arr string_of_expr_stmt])
    | PAREN_EXPR e ->
            make_json_kv "ParenExpr" (string_of_expr_stmt e)
    | RANGE (a, b) ->
            make_json_kv "RangeOp" (make_json_kvs ["start";"end"]
                                                  [string_of_atom a; string_of_atom b]);;

