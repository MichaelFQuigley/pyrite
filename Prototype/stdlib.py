import llvmlite.binding
from llvmlite import ir
from llvmlite.ir import Constant, IntType, DoubleType, ArrayType

list_node_t   = ir.IdentifiedStructType(None, 'struct.list_node')
list_struct_t = ir.IdentifiedStructType(None, 'struct.list')

class StdLib:
    def __init__(self, lib_file, module):
        self.std_mod = None
        self.func_dict = {
                'list_init':
                ir.Function(module,
                    ir.FunctionType(ir.PointerType(list_struct_t), tuple()),
                    'list_init'),
                'list_add_int':
                ir.Function(module,
                    ir.FunctionType(ir.VoidType(), 
                        (ir.PointerType(list_struct_t), IntType(64))),
                    'list_add_int'),
                'list_add_float':
                ir.Function(module,
                    ir.FunctionType(ir.VoidType(), 
                        (ir.PointerType(list_struct_t), DoubleType())),
                    'list_add_float'),
                'print_list':
                ir.Function(module,
                    ir.FunctionType(ir.VoidType(), 
                        (ir.PointerType(list_struct_t),)),
                    'print_list'),
                'list_add_ptr':
                ir.Function(module,
                    ir.FunctionType(ir.VoidType(), 
                        (ir.PointerType(list_struct_t),ir.PointerType(IntType(8)))),
                    'list_add_ptr'),
                'list_add_bool':
                ir.Function(module,
                    ir.FunctionType(ir.VoidType(), 
                        (ir.PointerType(list_struct_t),IntType(8))),
                    'list_add_bool'),
                }
        self.typesMap = {
                'int': ir.IntType(64),
                'bool': ir.IntType(1),
                'float': ir.DoubleType(),
                'void': ir.VoidType(),
                'string': ir.PointerType(ir.IntType(8)),
                'list'  : ir.PointerType(list_struct_t),
                }
    def get_funcs(self):
        return self.func_dict

    def getType(self, type_name, fail_message= ' is not a known type.'):
        type_str = str(type_name)
        assert type_str in self.typesMap, type_str + fail_message
        return self.typesMap[type_str]
       

#stdLibModule = StdLib('stdlib/stdlib.ll')


