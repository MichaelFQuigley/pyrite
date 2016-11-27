type atom = 
    | LIT of string
    | RANGE of atom * atom
    | PAREN_EXPR of expr_stmt
    | ID of string * trailer array
and trailer = 
    | FCALL of expr_stmt array
    | INDEX of expr_stmt
    (* format of DOT: '.' identifier *)
    | DOT of string
and expr_stmt = 
    | BINOP of string * expr_stmt * expr_stmt
    | UNOP of string * atom
    (*format of if: test_expr * array(stmts_true, [stmts_false])*)
    | IF of expr_stmt * simple_stmt array
    (*format of for loop, index_var * iterator * stmts*)
    | LIST of expr_stmt array
    | LIST_GEN of expr_stmt * string * atom
    | ATOMOP of atom
    | FUNCDEF of func_def
    (*BRAC_EXPR represents multiple expression statements inside curly braces*)
    | BRAC_EXPR of simple_stmt array
and func_def = 
    | PLAIN_FUNC of string * func_proto * simple_stmt
and class_def =
    | PLAIN_CLASSDEF of string * class_member array
and class_member =
    | CLASS_FUNC of func_def
    | CLASS_FIELD of var_def
    | CLASS_CLASS of class_def
and simple_stmt = 
    | EXPROP of expr_stmt
    | VARDEF of var_def
    | FOR of string * atom * simple_stmt
    | WHILE of expr_stmt * simple_stmt
    | CLASSDEF of class_def
and var_def =
    | VDEFINITION of typed_arg * expr_stmt
    | VDEFINITION_INFER of string * expr_stmt
    | VDECLARATION of typed_arg
and typed_arg = 
    (*format of typedarg is name * type*)
    | TYPEDARG of string * type_definition
and type_definition = 
    | SIMPLE_TYPE of string
    | FUNC_TYPE of func_proto
    | LIST_TYPE of type_definition
and func_proto = 
    (*format of FUNC_PROTO is array(arg1, arg2, ...) * ret_type*)
    | FUNC_PROTO of typed_arg array * type_definition
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
    | FUNC_TYPE func_proto -> make_json_kv "func_type" (string_of_prototype func_proto)
    | LIST_TYPE list_type -> make_json_kv "list_type" (string_of_type_definition list_type)
and string_of_prototype ast = 
    match ast with
    | FUNC_PROTO (args_arr, ret_type) -> make_json_kv "FuncProto" (make_json_kvs ["args"; "ret_type"] 
                                                                                 [(make_json_arr args_arr string_of_typed_arg); string_of_type_definition ret_type])

and string_of_simple_stmt ast = 
    match ast with
    | EXPROP op -> 
            make_json_kv "ExprOp" (string_of_expr_stmt op)
    | VARDEF vdef ->
            make_json_kv "VarDef" (string_of_var_def vdef)
    | FOR (loop_var, itt, stmt) -> 
            make_json_kv "ForOp" (make_json_kvs ["loop_var"; "itt" ; "body"] 
            ["\""^loop_var^"\""; (string_of_atom itt); (string_of_simple_stmt stmt)])
    | WHILE (header, stmt) ->
            make_json_kv "WhileOp" (make_json_kvs ["header"; "body"]
            [string_of_expr_stmt header; string_of_simple_stmt stmt])
    | CLASSDEF def ->
        make_json_kv "ClassDef" (string_of_class_def def)
and string_of_expr_stmt ast = 
    match ast with
    | UNOP (op, expr) ->
            make_json_kv "UnOp" (make_json_kvs ["op"; "atom"]
                                               ["\""^op^"\""; string_of_atom expr])
    | BINOP (op, expa, expb) -> 
            make_json_kv "BinOp" (make_json_kvs ["op"; "lhs"; "rhs"] 
                                                ["\""^op^"\""; (string_of_expr_stmt expa); (string_of_expr_stmt expb)])
    | ATOMOP at -> 
            make_json_kv "AtomOp" (string_of_atom at)
    | IF (test_expr, stmts_arr) -> 
            make_json_kv "IfOp" (make_json_kvs ["test"; "bodies"]
                                               [(string_of_expr_stmt test_expr); make_json_arr stmts_arr string_of_simple_stmt])
    | LIST elements ->
            make_json_kv "ListOp" (make_json_arr elements string_of_expr_stmt)
    | FUNCDEF def ->
        make_json_kv "FuncDef" (string_of_func_def def)
    | LIST_GEN (expr, loop_var, itt) ->
            make_json_kv "ListGen" (make_json_kvs ["element_expr"; "loop_var"; "itt"]
                                                  [string_of_expr_stmt expr; "\""^loop_var^"\""; string_of_atom itt])
    | BRAC_EXPR exprs -> (make_json_kv "BracExpr" (make_json_arr exprs string_of_simple_stmt))
and string_of_func_def ast =
    match ast with
    | PLAIN_FUNC (name, proto, stmt) ->
            make_json_kv "PlainFunc" (make_json_kvs ["name"; "header"; "simple_stmt"]
                                ["\""^name^"\""; string_of_prototype proto; string_of_simple_stmt stmt])
and string_of_typed_arg ast = 
    match ast with 
    | TYPEDARG (name, t) -> 
            make_json_kv "TypedArg" (make_json_kvs ["name"; "type"]
                                                   ["\""^name^"\""; string_of_type_definition t])
and string_of_trailers ast = 
    match ast with
    | INDEX expr ->
            make_json_kv "Index" (string_of_expr_stmt expr)
    | FCALL exprs_arr -> 
            make_json_kv "FCall" (make_json_kv "args" (make_json_arr exprs_arr string_of_expr_stmt))
    | DOT ident ->
            make_json_kv "Dot" ("\""^ident^"\"")
and string_of_var_def ast =
    match ast with
    | VDEFINITION (t, expr) ->
            make_json_kv "definition" (make_json_kvs ["var"; "expr"]
                                                 [string_of_typed_arg t; string_of_expr_stmt expr])
    | VDEFINITION_INFER (name, expr) ->
            make_json_kv "definition_infer" (make_json_kvs ["name"; "expr"]
                                                 ["\""^name^"\""; string_of_expr_stmt expr])
    | VDECLARATION t ->
            make_json_kv "declaration" (string_of_typed_arg t)
and string_of_class_def ast =
    match ast with
    | PLAIN_CLASSDEF (name, members) ->
        make_json_kv "PlainClassDef"
          (make_json_kvs ["name"; "members"]
                         ["\""^name^"\""; (make_json_arr members string_of_class_members)])
and string_of_class_members ast =
    match ast with
    | CLASS_FUNC func ->
        make_json_kv "ClassFunc" (string_of_func_def func) 
    | CLASS_FIELD field ->
        make_json_kv "ClassField" (string_of_var_def field)
    | CLASS_CLASS classs ->
        make_json_kv "ClassClass" (string_of_class_def classs)

and string_of_atom ast = 
    match ast with
    | LIT a -> 
            make_json_kv "Lit" a
    | ID (name, trailers) -> 
            make_json_kv "Id" (make_json_kvs ["name"; "trailers"]
                                             ["\""^name^"\""; (make_json_arr trailers string_of_trailers)])
    | PAREN_EXPR e ->
            make_json_kv "ParenExpr" (string_of_expr_stmt e)
    | RANGE (a, b) ->
            make_json_kv "RangeOp" (make_json_kvs ["start";"end"]
                                                  [string_of_atom a; string_of_atom b]);;

