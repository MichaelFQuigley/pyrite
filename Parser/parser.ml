let op_precedence:(string, int) Hashtbl.t = Hashtbl.create 5
let precedence op = try Hashtbl.find op_precedence op with Not_found -> -1

(*parse_top is starting point*)
let rec parse_top = parser
    | [< e=parse_stmts >] -> e
    | [< >] -> raise (Stream.Error "Parse error at top level")
and parse_initial = 
    (
    Util.debug_print "in parse_initial";
    parser
    | [< stream >] ->
    (match Stream.peek stream with
        | Some Token.LIT _ | Some Token.IDENT _ | Some Token.LPAREN ->  
                Ast.ATOMOP (parse_atom stream)
        | _ ->
            (parser
            (*unary ops*)
            | [< 'Token.PUNCT "-"; atom >] ->
                Ast.UNOP ("-", (parse_atom atom))
            (*if stmt*)
            | [< 'Token.KWD "if"; 
              'Token.LPAREN;  test=parse_expr; 'Token.RPAREN ?? "Expected ')' in if stmt.";
              t=parse_simple_stmt;
               stream >] ->
            (*else clause of if stmt*)
                (match Stream.peek stream with
                    | Some (Token.KWD "else") -> 
                        begin
                            Stream.junk stream; 
                            begin parser
                                | [< e=parse_simple_stmt >] 
                                  -> Ast.IF (test, (Array.of_list [t; e]))
                                | [< _ >] -> raise (Stream.Error "Else stmt requires '{}'.")
                            end stream
                        end
                    | _ -> Ast.IF (test, Array.of_list [t]))
            | [< 'Token.KWD "func"; stream >] -> 
                Ast.FUNCDEF (parse_plain_func_def stream)
              | [<'Token.LSQ; stream>] ->
                       (*empty list*)
                      (if Stream.peek stream = Some (Token.RSQ) then
                          (Stream.junk stream; Ast.LIST [||])
                       (*non-empty list*)
                       else (parser
                           | [< expr=parse_expr; stream >] ->
                                        (match Stream.peek stream with
                                            (*list with multiple elements*)
                                            | Some (Token.PUNCT ",") ->
                                                    (Stream.junk stream;
                                                     let result = Ast.LIST
                                                      (Array.of_list
                                                       (expr::(List.rev (parse_args [] stream)))) in(
                                                      Stream.junk stream;
                                                      result))
                                             (*list with single element*)
                                             | Some (Token.RSQ) ->
                                                    (Stream.junk stream;
                                                      Ast.LIST [|expr|])
                                             (*list generator*)
                                             | Some (Token.KWD "for") ->
                                                     (parse_list_generator expr stream)
                                             | _ -> raise (Failure "Invalid list syntax."))) stream)
            (*Block statement*)
            | [< 'Token.LBRAC; brac_stmts_list=parse_stmts_helper; 'Token.RBRAC
                 ?? "Expected '}' in bracketed expression." >] -> 
                    Ast.BRAC_EXPR (Array.of_list brac_stmts_list)
            ) stream))
and parse_plain_class_def = 
    parser
      | [<'Token.IDENT class_name; 'Token.LBRAC; members=parse_class_members; 'Token.RBRAC>] ->
            Ast.PLAIN_CLASSDEF (class_name, (Array.of_list members))
and parse_plain_func_def stream = 
    let parse_func_proto_and_body name stream =
         (parser
             | [< proto=parse_func_proto; stmt=parse_simple_stmt >] ->
                 Ast.PLAIN_FUNC (name, proto,  stmt)) stream
        in
    (match Stream.peek stream with
        | Some (Token.LPAREN) ->
                parse_func_proto_and_body "" stream
        | Some (Token.IDENT name) ->
                (Stream.junk stream;
                parse_func_proto_and_body name stream)
        | _ -> raise (Failure "Invalid function definition."))
and parse_list_generator element_expr = 
    parser
        | [<'Token.KWD "for"; 'Token.LPAREN; 'Token.IDENT gen_var; 'Token.KWD "in"; itt=parse_atom; 'Token.RPAREN; 'Token.RSQ>] ->
            (Ast.LIST_GEN (element_expr, gen_var, itt))
        | [<>] -> (raise (Failure "Invalid list generator syntax."))
and parse_atom =
    let check_for_dots lhs stream = 
        (match Stream.peek stream with
            | Some (Token.PUNCT "..") -> (Stream.junk stream; Ast.RANGE (lhs, parse_atom stream))
            | _ -> lhs) in 
    parser
    | [< 'Token.LIT n; stream >] -> (check_for_dots (Ast.LIT n) stream)
    | [< 'Token.IDENT id; stream >] -> (check_for_dots (parse_ident id stream) stream)
    | [< 'Token.LPAREN; e=parse_expr ; 'Token.RPAREN ?? "Expected ')'."; stream >] -> 
            (check_for_dots (Ast.PAREN_EXPR e) stream)
and parse_var_def = parser
    | [< 'Token.IDENT id; stream >] ->
            (match Stream.next stream with
                | (Token.PUNCT ":") ->
                        (parser
                            | [< t=parse_type_definition; stream >] ->
                                    (match Stream.peek stream with
                                        | Some (Token.PUNCT "=") ->
                                                Stream.junk stream;
                                                Ast.VDEFINITION (Ast.TYPEDARG (id, t), parse_expr stream)
                                        | _ ->
                                                Ast.VDECLARATION (Ast.TYPEDARG (id, t)))) stream
                | (Token.PUNCT "=") ->
                        Ast.VDEFINITION_INFER (id, (parse_expr stream))
                | _ -> raise (Failure "Invalid variable definition."))
     | [< >] -> raise (Failure "Invalid variable definition.")
(*    | [< v=parse_typed_arg; 'Token.PUNCT "="; e=parse_expr >] -> Ast.VARDEF (v,e)*)
and parse_typed_arg = parser
    | [< 'Token.IDENT id ; 'Token.PUNCT ":"; t=parse_type_definition >] -> Ast.TYPEDARG (id, t) 
and parse_func_proto = 
       let rec typed_arg_helper curr_args stream = 
             begin 
                 parser 
                 | [< arg=parse_typed_arg; stream >] ->
                         (parser
                         | [< 'Token.PUNCT ","; arg=typed_arg_helper (arg::curr_args) >] -> arg
                         | [< >] -> arg::curr_args) stream
                 | [< >] -> curr_args
             end stream in
             (parser 
                 [< 'Token.LPAREN; args=typed_arg_helper []; 'Token.RPAREN ?? "Expected ) in function type definition."; stream >] ->
                     (match Stream.peek stream with
                        | Some (Token.PUNCT "->") ->
                                (Stream.junk stream; Ast.FUNC_PROTO (Array.of_list (List.rev args), 
                                                                                    (parse_type_definition stream)))
                        | _ ->
                                Ast.FUNC_PROTO (Array.of_list (List.rev args), 
                                                              (Ast.SIMPLE_TYPE "Void"))))
and parse_type_definition = parser
    | [< stream >] -> 
          (match Stream.peek stream with
          | Some (Token.IDENT typ) -> 
                  begin
                      Stream.junk stream;
                      Ast.SIMPLE_TYPE typ
                  end
          | Some (Token.LPAREN) ->
                  Ast.FUNC_TYPE (parse_func_proto stream)
          | Some (Token.LSQ) ->
                  (parser
                      | [<'Token.LSQ; v=parse_type_definition; 'Token.RSQ>] ->
                              Ast.LIST_TYPE v
                      | [<>] -> raise (Failure "List type declaration failed to parse!")) stream
          | _ -> raise (Failure "Arg type did not parse."))
and parse_class_members =
  parser
    | [< 'Token.KWD "let";  e=parse_var_def; stream>] ->
        (Ast.CLASS_FIELD e)::(parse_class_members stream)
    | [< 'Token.KWD "func";  e=parse_plain_func_def; stream>] ->
        (Ast.CLASS_FUNC e)::(parse_class_members stream)
    | [< 'Token.KWD "class";  e=parse_plain_class_def; stream>] ->
        (Ast.CLASS_CLASS e)::(parse_class_members stream)
    | [<>] -> []

and parse_stmts_helper =
    parser
        | [< simple_stmt=parse_simple_stmt; stream >] 
             -> simple_stmt::(parse_stmts_helper stream)
        | [< >] -> []
and parse_stmts = parser
    [< stream >] ->
      Ast.STMTS (Array.of_list (parse_stmts_helper stream))
and parse_simple_stmt = parser
    | [< 'Token.KWD "let"; e=parse_var_def>] -> Ast.VARDEF e
    | [< e=parse_expr >] -> (Ast.EXPROP e)
    (*for loop*)
    | [< 'Token.KWD "for"; 
         'Token.LPAREN; 'Token.IDENT loop_var; 'Token.KWD "in"; itt=parse_atom; 'Token.RPAREN;
              simple_stmt=parse_simple_stmt>] ->
                   Ast.FOR (loop_var, itt, simple_stmt)
    | [< 'Token.KWD "while"; 'Token.LPAREN; header=parse_expr;  'Token.RPAREN; stmt=parse_simple_stmt>] ->
                   Ast.WHILE (header, stmt)
    | [< 'Token.KWD "class"; stream >] ->
        Ast.CLASSDEF (parse_plain_class_def stream)
and parse_expr = 
    begin
    Util.debug_print "in parse_expr";
    parser
    | [< lhs=parse_initial; stream >] -> (parse_bin_rhs 0 lhs stream)
    end
(*parse_args is used to parse args for a function call*)
and parse_args curr_args = 
    begin
    Util.debug_print "in parse_args";
    parser
    | [< a=parse_expr; stream >] -> 
            (parser
            | [< 'Token.PUNCT ","; a=parse_args (a::curr_args) >] -> a
            | [< >] -> a :: curr_args) stream
    | [< >] -> curr_args
    end
and parse_ident id = 
    begin
        Util.debug_print "in parse_ident";
        parser
            | [< stream >] -> Ast.ID (id, Array.of_list (parse_trailers [] stream))
    end
and parse_trailers trailers_list = 
    parser
        | [< 'Token.LPAREN ; 
            args=parse_args []; 
            'Token.RPAREN ?? "Expected ')' for fcall.";
            stream>] 
            -> (Ast.FCALL (Array.of_list (List.rev args)))::(parse_trailers trailers_list stream)
        | [< 'Token.LSQ;
             expr=parse_expr;
             'Token.RSQ ?? "Expected ']' for index.";
             stream >]
            -> (Ast.INDEX expr)::(parse_trailers trailers_list stream)
        | [< >] -> trailers_list
(*parse_bin_rhs parses binary operations*)
and parse_bin_rhs prec lhs stream =
    let make_augassign op lhs stream =
        (Stream.junk stream; 
        Ast.BINOP ("=", lhs, (Ast.BINOP (op, lhs, parse_expr stream))))
    in
    begin
    Util.debug_print "in parse_bin_rhs";
    match Stream.peek stream with
    (*AugAssign ops bind really loosely, so no precedence is needed to be checked*)
    | Some (Token.PUNCT "+=") -> make_augassign "+" lhs stream
    | Some (Token.PUNCT "/=") -> make_augassign "/" lhs stream
    | Some (Token.PUNCT "*=") -> make_augassign "*" lhs stream
    | Some (Token.PUNCT "%=") -> make_augassign "%" lhs stream
    | Some (Token.PUNCT "|=") -> make_augassign "|" lhs stream
    | Some (Token.PUNCT "&=") -> make_augassign "&" lhs stream
    | Some (Token.PUNCT "^=") -> make_augassign "^" lhs stream
    | Some (Token.PUNCT p) when Hashtbl.mem op_precedence p ->
        let curr_precedence = precedence p in
        if curr_precedence < prec then lhs else begin
            Stream.junk stream;
            let rhs = parse_initial stream in
            let rhs = 
                match Stream.peek stream with
                | Some (Token.PUNCT pp) ->
                    let new_prec = precedence pp in
                    if curr_precedence < new_prec then
                        parse_bin_rhs (curr_precedence + 1) rhs stream
                        else rhs
                | _ -> rhs in
            let lhs = Ast.BINOP (p, lhs, rhs) in parse_bin_rhs prec lhs stream
        end
    | _ -> lhs
    end
