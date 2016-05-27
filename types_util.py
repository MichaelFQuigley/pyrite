from llvmlite import ir

class LambdaFunc:
    def __init__(self, name, function, lambda_node, args):
        self.name = name
        self.function = function
        self.lambda_node = lambda_node
        self.args = args
        self.vals = []
    #vals is a list of tuples [(name, value, size, index),...]
    #def set_closure_vals(self, vals):
     


def get_string_type(string):
    #string = string.replace('\\n','\n')
    n = (len(string) + 1)
    buf = bytearray((' ' * n).encode('ascii'))
    buf[-1] = 0
    buf[:-1] = string.encode('utf-8')
    return ir.Constant(ir.ArrayType(ir.IntType(8), n), buf)


def get_type_size(var_type):
   print type(var_type)
   if type(var_type) == ir.DoubleType: 
       return 8
   if type(var_type) == ir.PointerType: 
       return 8
   if type(var_type) == ir.IntType: 
       return (var_type.width + 7) // 8
   assert False, str(var_type) + " has unknown size"


