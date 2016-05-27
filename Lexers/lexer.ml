let rec lex = parser
    | [< ' (' '| '\n' | '\r' | '\t'); stream >] -> lex stream
    | [<' ('#'); stream>] -> lex_comments stream
    | [<' ('('); stream>] -> [<'Token.LPAREN; lex stream>]
    | [<' (')'); stream>] -> [<'Token.RPAREN; lex stream>]
    | [<' ('{'); stream>] -> [<'Token.LBRAC; lex stream>]
    | [<' ('}'); stream>] -> [<'Token.RBRAC; lex stream>]
    | [<' ('"'); stream>] -> let buffer = Buffer.create 1 in
                                    lex_string buffer stream;
    | [<' ('0' .. '9' as c); stream>] ->  
                                let buffer = Buffer.create 1 in
                                          Buffer.add_char buffer c;
                                          lex_number buffer stream
    | [<' ('A' .. 'Z' | 'a' .. 'z' | '_' as c); stream>] -> 
                                let buffer = Buffer.create 1 in 
                                        Buffer.add_char buffer c;
                                        lex_ident buffer stream

    | [<' ('='); stream>] ->
                           (match (Stream.peek stream) with
                            | (Some '>') ->  Stream.next stream; [<'Token.PUNCT "=>"; lex stream>]
                            | (Some '<') ->  Stream.next stream; [<'Token.PUNCT "=<"; lex stream>]
                            | (Some '=') ->  Stream.next stream; [<'Token.PUNCT "=="; lex stream>]
                            | _ -> [<'Token.PUNCT "="; lex stream>]);
    | [<' ('-'); stream>] ->
                            (match (Stream.peek stream) with
                            |  Some '>' ->  Stream.next stream; [<'Token.PUNCT "->"; lex stream>]
                            | _ -> [<'Token.PUNCT "-"; lex stream>]);
    | [<' ('+'|'\\'|'*'|'%'
            |','|':'|'.'|';'
            |'|'|'&'|'^'|'~'
            |'>'|'<' as c); stream>] 
        -> [< 'Token.PUNCT (Char.escaped c); lex stream>]
    (*| [<>] -> [< 'Token.EOF >]*)
    | [<>] -> [< >]

and lex_comments = parser
    | [<' ('\n'); stream >] -> lex stream
    | [<'c; stream >] -> lex_comments stream
    | [<>] -> [<>] 

and lex_string buffer = parser
    | [<' ('"'); stream>] -> [<'Token.LIT ("\""^(Buffer.contents buffer)^"\""); lex stream>]
    | [<'c; stream>] -> Buffer.add_char buffer c;
                        lex_string buffer stream
    | [<>] -> raise (Failure "Unterminated string.")

and lex_number buffer = parser
    | [<' ('0' .. '9' as c); stream>] -> Buffer.add_char buffer c;
                                         lex_number buffer stream
    | [<' ('.' as c); stream>] -> Buffer.add_char buffer c;
                                    lex_float buffer stream
    | [<' ('a' .. 'z' | 'A' .. 'Z') ; _>] -> raise (Failure "Lexer error")
    | [<stream>] ->
        [<'Token.LIT (Buffer.contents buffer); lex stream>]
    | [<>] -> [<'Token.LIT (Buffer.contents buffer)>]

and lex_float buffer = parser
    | [<' ('0' .. '9' as c); stream>] -> Buffer.add_char buffer c;
                                         lex_float buffer stream
    | [<' ('a' .. 'z' | 'A' .. 'Z') ; _>] -> raise (Failure "Lexer error")
    | [<stream>] ->
        [<'Token.LIT (Buffer.contents buffer); lex stream>]
    | [<>] -> [<'Token.LIT (Buffer.contents buffer)>]
and lex_ident buffer = parser
    | [< ' ('A' .. 'Z' | 'a' .. 'z' | '0' .. '9' | '_' as c); stream >] ->
        Buffer.add_char buffer c;
        lex_ident buffer stream
    | [<stream=lex>] ->
          match Buffer.contents buffer with
          | (
            "and"
            |"else"
            |"false"
            |"func"
            |"if"
            |"in"
            |"let"
            |"loop"
            |"not"
            |"or"
            |"return"
            |"true"
            |"xor"
            ) -> [<'Token.KWD (Buffer.contents buffer); stream>]
          | ident -> [<'Token.IDENT (Buffer.contents buffer); stream>];
;;


