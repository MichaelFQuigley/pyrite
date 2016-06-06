let op_precedence:(string, int) Hashtbl.t = Hashtbl.create 5
let precedence op = try Hashtbl.find op_precedence op with Not_found -> -1

(*parse_top is starting point*)
let rec parse_top = parser
    | [< e=parse_stmts >] -> e
    | [< >] -> raise (Stream.Error "Parse error at top level")
and parse_initial = 
    begin 
    Util.debug_print "in parse_initial";
    parser
    | [< 'Token.LIT n >] -> Ast.ATOMOP (Ast.LIT n)
    | [< 'Token.IDENT id; stream>] -> Ast.ATOMOP (parse_ident id stream)
    | [< 'Token.LPAREN; e=parse_expr ; 'Token.RPAREN ?? "Expected ')'.">] -> e
    (*if stmt*)
    | [< 'Token.KWD "if"; 
      'Token.LPAREN;  test=parse_expr; 'Token.RPAREN ?? "Expected ')' in if stmt.";
      'Token.LBRAC;
      t=parse_stmts;
      'Token.RBRAC ?? "expected '}' in if stmt"; stream >] ->
    (*else clause of if stmt*)
        (match Stream.peek stream with
            | Some (Token.KWD "else") -> 
                begin
                    Stream.junk stream; 
                    begin parser
                        | [< 'Token.LBRAC; e=parse_stmts; 'Token.RBRAC ?? "Expected '}' in else.">] 
                          -> Ast.IF (test, (Array.of_list [t; e]))
                        | [< _ >] -> raise (Stream.Error "Else stmt requires '{}'.")
                    end stream
                end
            | _ -> Ast.IF (test, Array.of_list [t]))
    (*loop statement*)
    | [< 'Token.KWD "loop"; stream>] -> 
            let rec loop_arg_helper end_token stream = 
                (match Stream.peek stream with
                    | Some tok when tok = end_token -> 
                            begin 
                                Stream.junk stream;
                                loop_arg_helper end_token stream
                            end
                    | _ ->
                            begin parser
                                | [< e=parse_expr; stream >] -> 
                                        e::(loop_arg_helper end_token stream)
                                | [<>] -> []
                            end stream)
            in begin parser
                [< 'Token.LPAREN; loop_args=loop_arg_helper (Token.PUNCT ";") ; 'Token.RPAREN;
                   'Token.LBRAC; stmts=parse_stmts; 'Token.RBRAC>] ->
                    (if    (((List.length loop_args) == 0) 
                        || ((List.length loop_args) == 1)
                        || ((List.length loop_args) == 3)) then
                        Ast.LOOP ((Array.of_list loop_args), stmts)
                    else 
                        raise (Stream.Error "Loop must have zero, one, or three arguments."))
            end stream
    | [< 'Token.KWD "func"; stream >] -> 
            let parse_func_proto_and_body name stream =
                 (parser
                            | [<proto=parse_func_proto; 
                                'Token.LBRAC; 
                                stmts=parse_stmts; 
                                'Token.RBRAC ?? "Expected '}' in function definition">] ->
                                    Ast.FUNCDEF (name, proto,  stmts)) stream
                in
            (match Stream.peek stream with
                | Some (Token.LPAREN) ->
                        parse_func_proto_and_body "" stream
                | Some (Token.IDENT name) ->
                        (Stream.junk stream;
                        parse_func_proto_and_body name stream)
                | _ -> raise (Failure "INvalid function definition."))
    end
and parse_var_def = parser
    | [< v=parse_typed_arg; 'Token.PUNCT "="; e=parse_expr>] -> Ast.VARDEF (v,e)
and parse_typed_arg = parser
    | [< 'Token.IDENT id ; 'Token.PUNCT ":"; t=parse_type_definition>] -> Ast.TYPEDARG (id, t) 
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
             [< 'Token.LPAREN; args=typed_arg_helper []; 'Token.RPAREN ?? "Expected ) in function type definition."; 
                'Token.PUNCT "->" ?? "Expected return type in function."; ret_type=parse_type_definition>] -> 
                    Ast.FUNC_PROTO (Array.of_list (List.rev args), ret_type))
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
          | _ -> raise (Failure "Arg type did not parse."))
and parse_stmts = parser
    [< stream >] -> Ast.STMTS (Array.of_list (parse_stmt stream))
and parse_stmt = parser
    | [< 'Token.KWD "let"; e=parse_var_def; stmts>] -> e::(parse_stmt stmts)
    | [< e=parse_expr; stmts >] -> (Ast.EXPROP e)::(parse_stmt stmts)
    | [< 'Token.KWD "return"; 
            'Token.LBRAC; 
            s=parse_stmts; 
            'Token.RBRAC ?? "Expected '}' for return stmt";
            stmts>] -> 
                (Ast.RETURN s)::(parse_stmt stmts)
    | [< >] -> []
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
        | [< 'Token.LPAREN ; args=parse_args []; 'Token.RPAREN ?? "Expected ')' for fcall." >] 
            -> Ast.FCALL (id, Array.of_list (List.rev args)) 
        | [< >] -> Ast.VARIABLE id
    end
(*parse_bin_rhs parses binary operations*)
and parse_bin_rhs prec lhs stream =
    let make_augassign op lhs stream =
        (Stream.junk stream; Ast.BINOP ("=", lhs, Ast.BINOP (op, lhs, parse_initial stream))) 
    in
    begin
    Util.debug_print "in parse_bin_rhs";
    match Stream.peek stream with
    (*AugAssign ops bind really loosely, so no precedence is needed to be checked*)
    | Some (Token.PUNCT "+=") -> make_augassign "+" lhs stream
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
