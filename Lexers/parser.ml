
let op_precedence:(string, int) Hashtbl.t = Hashtbl.create 5
let precedence op = try Hashtbl.find op_precedence op with Not_found -> -1

let rec parse_top = parser
    | [< e=parse_expr >] -> Ast.EXPROP e
and parse_atom = 
    begin 
    Util.debug_print "in parse_atom";
    parser
    | [< 'Token.LIT n >] -> Ast.ATOMOP (Ast.LIT n)
    | [< 'Token.IDENT id; stream>] -> Ast.ATOMOP (parse_ident id stream)
    | [< 'Token.LPAREN; e=parse_expr ; 'Token.RPAREN ?? "expected ')'">] -> e
    | [< 'Token.EOF >] -> raise (Stream.Error "EOF")
    | [< >] -> Ast.ATOMOP (Ast.LIT "TEST") (*raise (Stream.Error "Invalid token in parse_atom.")*)
    end
and parse_expr = 
    begin
    Util.debug_print "in parse_expr";
    parser
    | [< lhs=parse_atom; stream >] -> (parse_bin_rhs 0 lhs stream)
    | [< >] -> raise (Stream.Error "Invalid")
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
            let rhs = parse_atom stream in
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

