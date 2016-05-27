
let op_precedence:(string, int) Hashtbl.t = Hashtbl.create 5
let precedence op = try Hashtbl.find op_precedence op with Not_found -> -1

let rec parse_top = parser
    | [< e=parse_stmts >] -> e
    | [< >] -> raise (Stream.Error "Parse error at top level")
and parse_initial = 
    begin 
    Util.debug_print "in parse_initial";
    parser
    | [< 'Token.LIT n >] -> Ast.ATOMOP (Ast.LIT n)
    | [< 'Token.IDENT id; stream>] -> Ast.ATOMOP (parse_ident id stream)
    | [< 'Token.LPAREN; e=parse_expr ; 'Token.RPAREN ?? "expected ')'">] -> e
    (*if stmt*)
    | [< 'Token.KWD "if"; 
      'Token.LPAREN;  test=parse_expr; 'Token.RPAREN ?? "expected ')' in if stmt";
      'Token.LBRAC;
      t=parse_expr;
      'Token.RBRAC ?? "expected '}' in if stmt"; stream >]
    (*else clause of if stmt*)
        ->  begin match Stream.peek stream with
            | Some (Token.KWD "else") -> begin
                Stream.junk stream; 
                begin parser
                    [< 'Token.LBRAC; e=parse_expr; 'Token.RBRAC ?? "expected '}' in else">] 
                      -> Ast.IF (Array.of_list [test; t; e])
                end stream
                end
            | _ -> Ast.IF (Array.of_list [test; t]) end
    | [< 'Token.KWD "loop";
      'Token.LPAREN; 
      a=parse_expr; 'Token.PUNCT ";";
      b=parse_expr; 'Token.PUNCT ";";
      c=parse_expr;
      'Token.RPAREN ?? "expected ')' in loop stmt";
      stmts=parse_expr>] 
        -> Ast.LOOP (a,b,c, stmts)
    end
and parse_stmts = parser
    [< stream >] -> Ast.STMTS (Array.of_list (parse_stmt stream))
and parse_stmt = parser
    | [< e=parse_expr; stmts >] -> (Ast.EXPROP e)::(parse_stmt stmts)
    | [< >] -> []
and parse_expr = 
    begin
    Util.debug_print "in parse_expr";
    parser
    | [< lhs=parse_initial; stream >] -> (parse_bin_rhs 0 lhs stream)
    end
and parse_args curr_args = 
    begin
    Util.debug_print "in parse_args";
    parser
    | [< a=parse_expr; stream >] -> 
            begin parser
                | [< 'Token.PUNCT ","; a=parse_args (a::curr_args) >] -> a
                | [< >] -> a :: curr_args
            end stream
    | [< >] -> curr_args
    end
and parse_ident id = 
    begin
    Util.debug_print "in parse_ident";
    parser
    | [< 'Token.LPAREN ; args=parse_args []; 'Token.RPAREN ?? "expected ')' for fcall" >] 
        -> Ast.FCALL (id, Array.of_list (List.rev args)) 
    | [< >] -> Ast.VARIABLE id
    end
and parse_bin_rhs prec lhs stream =
    begin
    Util.debug_print "in parse_bin_rhs";
    match Stream.peek stream with
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

