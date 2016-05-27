let op_precedence:(string, int) Hashtbl.t = Hashtbl.create 5
let precedence op = try Hashtbl.find op_precedence op with Not_found -> -1

let rec parse_top = parser
    | [< e >] -> Ast.EXPROP (parse_expr e)
and parse_atom = parser
    | [< 'Token.LIT n >] -> Ast.ATOMOP (Ast.LIT n)
    | [< 'Token.IDENT id; stream>] -> Ast.ATOMOP (parse_ident id stream)
    | [< 'Token.LPAREN; e ; 'Token.RPAREN ?? "expected ')'">] -> parse_expr e
    | [< 'Token.EOF >] -> raise (Stream.Error "EOF")
    | [< >] -> raise (Stream.Error "Invalid token.")
and parse_expr = parser
    | [< lhs; stream >] -> (parse_bin_rhs 0 (parse_atom lhs) stream)
    | [< >] -> raise (Stream.Error "Invalid")
and parse_args curr_args = parser
    | [< a=parse_expr; stream >] -> 
            begin parser
                | [< 'Token.PUNCT ","; args_rest >] -> parse_args (a::curr_args) args_rest
                | [< >] -> a :: curr_args
            end stream
    | [< >] -> curr_args
and parse_ident id = parser
    | [< 'Token.LPAREN ; args; 'Token.RPAREN ?? "expected ')'" >] 
        -> Ast.FCALL (id, Array.of_list (List.rev (parse_args [] args))) 
    | [< >] -> Ast.VARIABLE id
and parse_bin_rhs prec lhs stream =
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


    (*| [< 'Token.LPAREN; e ; 'Token.RPAREN >] -> Ast.ATOM (parse_expr e)*)
