import grako
from myParse import *
from grako.model import ModelBuilderSemantics, Node, NodeWalker
from llvmlite import ir
from llvmlite import binding
from llvmlite.ir import Constant, IntType, DoubleType, ArrayType
from parse_util import *
from stdlib import *

output_filename = 'foo.ll'

llvm_gen = []

module      = ir.Module(name=output_filename, triple=binding.get_default_triple())
globalScope = ir.Function(module, ir.FunctionType(IntType(32),[]), name='main')
globalBlock = globalScope.append_basic_block()
builder     = ir.IRBuilder(globalBlock)

named_values = {}

stdLibModule = StdLib('stdlib/stdlib.ll', module)
#print functionsDict

functionsDict = {
        'print':
            ir.Function(module, 
                        ir.FunctionType(IntType(32), 
                                (ir.PointerType(IntType(8)),),
                                var_arg=True),
                        'printf'),
        }

functionsDict.update(stdLibModule.get_funcs())


class MyNodeWalker(NodeWalker):
    
    def handle_rhs_ops(self, lhs, rhs, opFnMap):
        i = 0
        opFn = None
        result = lhs
        for el in rhs:
            if i % 2 == 0:
                for opStr in opFnMap:
                    if str(el) == opStr:
                        opFn = opFnMap[opStr]
                        break
            assert opFn is not None, 'opFn is not correct'
            if i % 2 != 0:
                result = opFn(result, self.walk(el))
                opFn = None
            i += 1
        return result

    def handle_arith_ops(self, lhs, rhs, opFnMap):
        opFn           = None
        result         = lhs
        promoteToFloat = str(lhs.type) == 'double'
        rhs_results    = []
        ops            = []

        i = 0
        for el in rhs:
            if i % 2 == 0:
                for opStr in opFnMap:
                    if str(el) == opStr:
                        ops.append(opStr)
                        break
            else:
                rhs_walk = self.walk(el)
                promoteToFloat = promoteToFloat if str(rhs_walk.type) != 'double' else True
                rhs_results.append(rhs_walk)
            i += 1

        opIndex = 0
        for el in rhs_results:
            if promoteToFloat:
                opFn = opFnMap[ops[opIndex]][1]
                #cast to double
                fixed_res = builder.sitofp(result, DoubleType())
                fixed_el  = builder.sitofp(el, DoubleType()) 
                result = opFn(fixed_res, fixed_el)
            else:
                opFn = opFnMap[ops[opIndex]][0]
                result = opFn(result, el)
            opIndex += 1
        return result

    def walk_Start(self, node):
        debug_print('in start')
        for el in node.stmts:
            self.walk(el)

    def walk_Stmt(self,node):
        debug_print("in stmt")
        for el in node.simple:
            llvm_gen.append(self.walk(el))

    def walk_SimpleStmt(self, node):
        debug_print("in simpleStmt")
        return self.walk(node.simple)
  
    def walk_Args(self, node):
        debug_print("in Args")
        result = [self.walk(node.arg1)]
        i = 0
        if node.argrest is not None:
            for el in node.argrest:
                if str(el) == ',':
                    continue
                result.append(self.walk(el))
        return result

    def walk_ListExpr(self, node):
        debug_print("in ListExpr")
        list_inst = builder.call(functionsDict['list_init'],[]) 
        if node.arguments is not None:
            args = self.walk(node.arguments)
            for arg in args:
                if str(arg.type) == 'i64':
                    builder.call(functionsDict['list_add_int'],[list_inst, arg]) 
                elif str(arg.type) == 'double':
                    builder.call(functionsDict['list_add_float'],[list_inst, arg]) 
                else:
                    assert False, "Type for list not implemented yet"


        return list_inst

    def walk_LoopStmt(self, node):
        assert False, "Loops not done yet"

    def walk_IfStmt(self,node):
        pass
 
    def walk_Comparison(self,node):
        debug_print('in Comparison')
        result = self.walk(node.lhs)
        for el in node.rhs:
            assert False, "Comparison not done yet"
            self.walk(el)
        debug_print(result)
        return result 

    def walk_AssignStmt(self,node):
        debug_print('in AssignStmt')
        lhs = str(node.lhs)
        #if node.rhs == '[]':
            #rhs = builder.call(functionsDict['list_init'],[]) 
        #else: 
        rhs = self.walk(node.rhs)
        if lhs not in named_values:
            named_values[lhs] =  builder.alloca(rhs.type, 
                                    name=module.get_unique_name(lhs))
        return builder.store(rhs,named_values[lhs])

    def walk_ExprStmt(self,node):
        debug_print('in ExprStmt')
        lhs = self.walk(node.lhs)
        return self.handle_rhs_ops(lhs, node.rhs, {'|':builder.or_})

    def walk_XorStmt(self,node):
        debug_print('in XorStmt')
        lhs = self.walk(node.lhs)
        return self.handle_rhs_ops(lhs, node.rhs, {'^':builder.xor})

    def walk_AndExpr(self,node):
        debug_print('in AndExpr')
        lhs = self.walk(node.lhs)
        return self.handle_rhs_ops(lhs, node.rhs, {'&':builder.and_})

    def walk_ShiftExpr(self,node):
        debug_print('in ShiftExpr')
        lhs = self.walk(node.lhs)
        debug_print([str(self.walk(el)) for el in node.rhs])
        return self.handle_rhs_ops(lhs, 
                node.rhs, 
                {'>>':builder.lshr, '<<':builder.shl})

    def walk_AdditionExpr(self,node):
        debug_print('in AdditionExpr')
        lhs = self.walk(node.lhs)
        return self.handle_arith_ops(lhs, 
                node.rhs, 
                {'+':[builder.add, builder.fadd], 
                    '-':[builder.sub,builder.fsub]})

    def walk_MultExpr(self,node):
        debug_print('in MultExpr')
        lhs = self.walk(node.lhs)
        result = self.handle_arith_ops(lhs, 
                node.rhs, 
                {'*':[builder.mul, builder.fmul], 
                    '/':[builder.sdiv, builder.fdiv]})
        return result

    def walk_AtomExpr(self,node):
        debug_print('in AtomExpr')
        if node.at:
            #check if it is a variable
            if node.at.name:
                return builder.load(named_values[str(node.at.name)], 
                        str(node.at.name))
            return self.walk(node.at)
        elif node.fcall:
            fname = str(list(node.fcall)[0])
            if fname in functionsDict:
                return builder.call(functionsDict[fname], 
                    [] if len(node.fcall) == 3 
                    else self.walk(node.fcall[2]))
            else:
                assert False, "Undeclared function: " + str(fname)
        else:
            assert False, "AtomExpr parse failed"


    def walk_CompOp(self,node):
        debug_print('in CompOp')
        assert False, "Comp Op not implemented"

    def walk_Atom(self, node):
        debug_print('in Atom')
        if node.bool or node.name:
            debug_print('in atom bool or name')
        elif node.num:
            return self.walk(node.num)
        elif node.string:
            node_str = sanitize_string(node.string[1])
            gv = ir.GlobalVariable(module,
                    ir.ArrayType(ir.IntType(8), 
                        len(node_str) + 1), module.get_unique_name('str'))
            gv.global_constant = True
            gv.initializer = get_string_type(node_str)
            result = builder.gep(gv,[Constant(IntType(32),0),
                Constant(IntType(32),0)], inbounds=True)
            return result
        else:
            assert False, 'Atom parse failed'

    def walk_Number(self,node):
        debug_print('in Number')
        if node.float:
            return Constant(DoubleType(), float(''.join(node.float)))
        elif node.uint:
            return Constant(IntType(64), int(node.uint))
        else:
            assert False, "Number parse failed"
filename = 'myTest.my'

parser = grammarParser(parseInfo=True, comments_re=';.*\n')
with open(filename) as f:
    text = f.read()
ast = parser.parse(
    text,
    'start',
    filename=filename,
    semantics=ModelBuilderSemantics(),
    parseInfo=True)
walker = MyNodeWalker()
walker.walk(ast)

builder.ret(Constant(IntType(32), int(0)))

with open(output_filename,'w') as f:
    f.write(str(module))

