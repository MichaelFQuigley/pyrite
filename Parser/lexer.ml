let rec lex = parser
    | [< ' (' '| '\n' | '\r' | '\t'); stream >] -> lex stream
    | [<' ('#'); stream>] -> lex_comments stream
    | [<' ('('); stream>] -> [<'Token.LPAREN; lex stream>]
    | [<' (')'); stream>] -> [<'Token.RPAREN; lex stream>]
    | [<' ('{'); stream>] -> [<'Token.LBRAC; lex stream>]
    | [<' ('}'); stream>] -> [<'Token.RBRAC; lex stream>]
    | [<' ('['); stream>] -> [<'Token.LSQ; lex stream>]
    | [<' (']'); stream>] -> [<'Token.RSQ; lex stream>]
    | [<' ('"'); stream>] -> let buffer = Buffer.create 1 in lex_string buffer stream;
    | [<' ('0' .. '9' as c); stream>] ->  
        let buffer = Buffer.create 1 in
              Buffer.add_char buffer c;
              (match c with
               | '0' -> (match Stream.peek stream with
                            | Some 'X' | Some 'x' -> Stream.junk stream; lex_hex buffer stream
                            | Some 'B' | Some 'b' -> Stream.junk stream; lex_bin buffer stream
                            | _ -> lex_number buffer stream)
               | _ -> lex_number buffer stream)
    | [<' ('A' .. 'Z' | 'a' .. 'z' | '_' as c); stream>] -> 
        let buffer = Buffer.create 1 in 
                Buffer.add_char buffer c;
                lex_ident buffer stream
    | [<' ('='); stream>] -> lex_eq_sign '=' stream
    | [<' ('-'); stream>] ->
        (match (Stream.peek stream) with
        |  Some '>' ->  Stream.next stream; [<'Token.PUNCT "->"; lex stream>]
        |  Some '=' ->  Stream.next stream; [<'Token.PUNCT "-="; lex stream>]
        | _ -> [<'Token.PUNCT "-"; lex stream>])
    | [<' ('!'); stream >] -> lex_eq_sign '!' stream
    | [<' ('<'); stream >] -> lex_eq_sign '<' stream
    | [<' ('>'); stream >] -> lex_eq_sign '>' stream
    | [<' ('+'); stream >] -> lex_eq_sign '+' stream
    | [<' ('/'); stream >] -> lex_eq_sign '/' stream
    | [<' ('*'); stream >] -> lex_eq_sign '*' stream
    | [<' ('%'); stream >] -> lex_eq_sign '%' stream
    | [<' ('|'); stream >] -> lex_eq_sign '|' stream
    | [<' ('&'); stream >] -> lex_eq_sign '&' stream
    | [<' ('^'); stream >] -> lex_eq_sign '^' stream
    | [<' ('.'); stream>] ->
            (match Stream.peek stream with
            | Some '.' -> (Stream.junk stream; [<'Token.PUNCT ".."; lex stream>])
            | _ -> [<'Token.PUNCT "."; lex stream>])
    | [<' (','|':'|';'|'~' as c); stream>] 
        -> [< 'Token.PUNCT (Char.escaped c); lex stream>]
    | [<>] -> [<>]
(*lex_eq_sign checks to see if the first character in the stream is first_punct
 * and the second is an equal sign. It creates the appropriate PUNCT token
 * then continues lexing.*)
and lex_eq_sign first_punct = parser
    | [< stream >] ->
        (match (Stream.peek stream) with
            | (Some '=') -> 
                    Stream.next stream; 
                    [<'Token.PUNCT ((Char.escaped first_punct)^(Char.escaped '=')); lex stream>]
            | _ -> [<'Token.PUNCT (Char.escaped first_punct); lex stream>])
and lex_comments = parser
    | [<' ('\n'); stream >] -> lex stream
    | [<'c; stream >] -> lex_comments stream
    | [<>] -> [<>] 
and lex_string buffer = parser
    | [<' ('"'); stream>] -> [<'Token.LIT (str_lit (Buffer.contents buffer)); lex stream>]
    | [<'c; stream>] -> Buffer.add_char buffer c;
                        lex_string buffer stream
    | [<>] -> raise (Failure "Unterminated string.")
and lex_bin buffer = parser
    | [<' ('0' | '1' as c); stream>] -> Buffer.add_char buffer c; lex_bin buffer stream
    | [<' ('_'); stream>] -> lex_bin buffer stream
    | [<' ('a' .. 'z' | 'A' .. 'Z' | '2' .. '9') ; _>] -> raise (Failure "Could not parse bin value.")
    | [<stream>] -> 
            [< 'Token.LIT (int_lit (Int64.to_string (Int64.of_string ("0b"^(Buffer.contents buffer))))); lex stream>]
    | [<>] -> 
            [< 'Token.LIT (int_lit (Int64.to_string (Int64.of_string ("0b"^(Buffer.contents buffer))))); lex stream>]
and lex_hex buffer = parser
    | [<' (('0' .. '9') | ('A' .. 'F') as c); stream>] -> Buffer.add_char buffer c; lex_hex buffer stream
    | [<' ('_'); stream>] -> lex_hex buffer stream
    | [<' ('g' .. 'z' | 'G' .. 'Z') ; _>] -> raise (Failure "Could not parse hex value.")
    | [<stream>] -> 
            [< 'Token.LIT (int_lit (Int64.to_string (Int64.of_string ("0x"^(Buffer.contents buffer))))); lex stream>]
    | [<>] ->
            [< 'Token.LIT (int_lit (Int64.to_string (Int64.of_string ("0x"^(Buffer.contents buffer))))); lex stream>]
and lex_number buffer = parser
    | [<' ('0' .. '9' as c); stream>] -> Buffer.add_char buffer c; lex_number buffer stream
    | [<' ('_'); stream>] -> lex_number buffer stream
    | [<' ('.' as c); stream>] -> 
            (match Stream.peek stream with
            | Some '.' -> (Stream.junk stream; 
                            [< 'Token.LIT (int_lit (Buffer.contents buffer)); 
                               'Token.PUNCT ".."; 
                               lex stream>])
            | _ -> Buffer.add_char buffer c; lex_float buffer stream)
    | [<' ('a' .. 'z' | 'A' .. 'Z') ; _>] -> raise (Failure "Could not parse decimal value.")
    | [<stream>] ->
        [<'Token.LIT (int_lit (Buffer.contents buffer)); lex stream>]
    | [<>] -> [<'Token.LIT (int_lit (Buffer.contents buffer))>]

and lex_float buffer = parser
    | [<' ('0' .. '9' as c); stream>] -> Buffer.add_char buffer c;
                                         lex_float buffer stream
    | [<' ('_'); stream>] -> lex_float buffer stream
    | [<' ('a' .. 'z' | 'A' .. 'Z') ; _>] -> raise (Failure "Could not parse float value.")
    | [<stream>] ->
        [<'Token.LIT (float_lit (Buffer.contents buffer)); lex stream>]
    | [<>] -> [<'Token.LIT (float_lit (Buffer.contents buffer))>]
and lex_ident buffer = parser
    | [< ' ('A' .. 'Z' | 'a' .. 'z' | '0' .. '9' | '_' as c); stream >] ->
        Buffer.add_char buffer c;
        lex_ident buffer stream
    | [<stream=lex>] ->
          begin 
          match Buffer.contents buffer with
          | (
            "else"
            |"func"
            |"if"
            |"in"
            |"let"
            |"for"
            |"while"
            |"return"
            |"class"
            ) -> [<'Token.KWD (Buffer.contents buffer); stream>]
          | ("true" | "false") -> [<'Token.LIT (bool_lit (Buffer.contents buffer)); stream>]
          | ident -> [<'Token.IDENT (Buffer.contents buffer); stream>];
          end
   | [< >] -> [< >]
and int_lit int_str = 
    ("\"i"^int_str^"\"")
and float_lit float_str = 
    ("\"f"^float_str^"\"")
and str_lit str_str = 
    ("\"s"^str_str^"\"")
and bool_lit bool_str = 
    ("\"b"^bool_str^"\"")
;;


