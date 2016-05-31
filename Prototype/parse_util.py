from llvmlite import ir

debug = False

def sanitize_string(string):
    return str(string).replace('\\n','\n')

def debug_print(string):
    if debug:
        print string
