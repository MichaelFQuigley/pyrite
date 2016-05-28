type atom = 
    | LIT of string
    | FCALL of string * expr_stmt array
    | PAREN_EXPR of expr_stmt
    | VARIABLE of string
and expr_stmt = 
    | BINOP of string * expr_stmt * expr_stmt
    (*format of if: test_expr * array(stmts_true, [stmts_false])*)
    | IF of expr_stmt * stmts array
    | LOOP of expr_stmt array * stmts
    | ATOMOP of atom
and simple_stmt = 
    | EXPROP of expr_stmt
    (*format of VARDEF is variable * expression assigned to variable*)
    | VARDEF of typed_arg * expr_stmt
and typed_arg = 
    (*format of typedarg is name * type*)
    | TYPEDARG of string * type_definition
and type_definition = 
    | SIMPLE_TYPE of string
    (*format of FUNC_TYPE is array(arg1, arg2, ...) * ret_type*)
    | FUNC_TYPE of typed_arg array * type_definition
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
and string_of_type_definition ast =
    match ast with
    | SIMPLE_TYPE typ -> make_json_kv "simple" ("\""^typ^"\"")
    | FUNC_TYPE (args_arr, ret_type) -> make_json_kv "func_type" (make_json_kvs ["args"; "ret_type"] 
                                                                                [(make_json_arr args_arr string_of_typed_arg); string_of_type_definition ret_type])
    | _ -> raise (Failure "Unimplemented type")
and string_of_simple_stmt ast = 
    match ast with
    | EXPROP op -> 
            make_json_kv "ExprOp" (string_of_expr_stmt op)
    | VARDEF (t, expr) ->
            make_json_kv "VarDef" (make_json_kvs ["var"; "expr"]
                                                 [string_of_typed_arg t; string_of_expr_stmt expr])
and string_of_expr_stmt ast = 
    match ast with
    | BINOP (op, expa, expb) -> 
            make_json_kv "BinOp" (make_json_kvs ["op"; "lhs"; "rhs"] 
                                                ["\""^op^"\""; (string_of_expr_stmt expa); (string_of_expr_stmt expb)])
    | ATOMOP at -> 
            make_json_kv "AtomOp" (string_of_atom at)
    | LOOP (exprs_arr, stmts) -> 
            make_json_kv "LoopOp" (make_json_kvs ["args"; "body"] 
                                                 [make_json_arr exprs_arr string_of_expr_stmt; (string_of_stmts stmts)])
    | IF (test_expr, stmts_arr) -> 
            make_json_kv "IfOp" (make_json_kvs ["test"; "bodies"]
                                               [(string_of_expr_stmt test_expr); make_json_arr stmts_arr string_of_stmts])
and string_of_typed_arg ast = 
    match ast with 
    | TYPEDARG (name, t) -> 
            make_json_kv "TypedArg" (make_json_kvs ["name"; "type"]
                                                   ["\""^name^"\""; string_of_type_definition t])
and string_of_atom ast = 
    match ast with
    | LIT a -> 
            make_json_kv "Lit" a
    | VARIABLE a -> 
            make_json_kv "Variable" ("\""^a^"\"")
    | FCALL (name,exprs_arr) -> 
            make_json_kv "Fcall" (make_json_kvs ["name"; "args"]
                                                ["\""^name^"\""; make_json_arr exprs_arr string_of_expr_stmt])
    | _ -> raise (Failure "Invalid ast.");;

