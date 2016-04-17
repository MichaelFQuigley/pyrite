from llvmlite import ir

debug = False

def sanitize_string(string):
    return str(string).replace('\\n','\n')

def get_string_type(string):
    #string = string.replace('\\n','\n')
    n = (len(string) + 1)
    buf = bytearray((' ' * n).encode('ascii'))
    buf[-1] = 0
    buf[:-1] = string.encode('utf-8')
    return ir.Constant(ir.ArrayType(ir.IntType(8), n), buf)

def debug_print(string):
    if debug:
        print string
