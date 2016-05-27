type atom = 
    | LIT of string
    | VARIABLE of string
    | FCALL of string * expr_stmt array
    | PAREN_EXPR of expr_stmt
and expr_stmt = 
    | BINOP of string * expr_stmt * expr_stmt
    | ATOMOP of atom;;

type stmt = 
    | EXPROP of expr_stmt;;

