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
currScope = {'parent':None, 'scope':None}#ir.Function(module, ir.FunctionType(IntType(32),[]), name='main')
globalBlock = None #currScope.append_basic_block()
builder     = None #ir.IRBuilder(globalBlock)

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
            for opStr in opFnMap:
                if str(el[0]) == opStr:
                    opFn = opFnMap[opStr]
                    break
            assert opFn is not None, 'opFn is not correct'
            result = opFn(result, self.walk(el[1]))
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
            for opStr in opFnMap:
                if str(el[0]) == opStr:
                    ops.append(opStr)
                    break
            rhs_walk = self.walk(el[1])
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

    def walk_TopLvl(self, node):
        debug_print('in TopLvl')
        for top_stmt in node.top:
            self.walk(top_stmt)

    def walk_Stmt(self,node):
        debug_print("in stmt")
        for el in node.simple:
            self.walk(el)

    def walk_SimpleStmt(self, node):
        debug_print("in simpleStmt")
        return self.walk(node.simple)
  
    def walk_Args(self, node):
        debug_print("in Args")
        result = [self.walk(node.arg1)]
        i = 0
        if node.argrest is not None:
            for el in node.argrest:
                result.append(self.walk(el[1]))
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
                elif str(arg.type) == 'i1':
                    int8_val = builder.zext(arg, IntType(8))
                    builder.call(functionsDict['list_add_bool'],[list_inst, int8_val]) 
                else:
                    builder.call(functionsDict['list_add_ptr'],[list_inst, arg]) 


        return list_inst

    def walk_LoopStmt(self, node):
        debug_print('in LoopStmt')
        if node.first_stmt: self.walk(node.first_stmt)
        top_of_loop    = builder.append_basic_block(module.get_unique_name())
        body_of_loop   = builder.append_basic_block(module.get_unique_name())
        bottom_of_loop = builder.append_basic_block(module.get_unique_name())
        
        builder.branch(top_of_loop)
        builder.position_at_end(top_of_loop)
        if node.second_stmt: 
            comparison = self.walk(node.second_stmt)
            assert str(comparison.type) == 'i1', "Wrong type for loop"
            builder.cbranch(comparison, body_of_loop, bottom_of_loop)

        builder.position_at_end(body_of_loop)
        self.walk(node.loop_block)
        if node.third_stmt: self.walk(node.third_stmt)
        builder.branch(top_of_loop)
        builder.position_at_end(bottom_of_loop)


    def walk_IfStmt(self,node):
        debug_print('in IfStmt')
        first_comp = self.walk(node.iif_comp)
        if node.eelse:
            if len(node.eelse) == 4:
                eelse_block = node.eelse[2]
            else:
                eelse_block = node.eelse[1]
            if_name    = module.get_unique_name()
            else_name  = module.get_unique_name()
            endif_name = module.get_unique_name()
            if_label = builder.append_basic_block(if_name)
            else_label = builder.append_basic_block(if_name)
            endif_label = builder.append_basic_block(if_name)

            builder.cbranch(first_comp, if_label, else_label)
            builder.position_at_end(if_label)
            self.walk(node.iif_stmt)
            builder.branch(endif_label)
            builder.position_at_end(else_label)
            self.walk(eelse_block)
            builder.branch(endif_label)
            builder.position_at_end(endif_label)
        #if-then case
        else:
            with builder.if_then(first_comp) as bbend:
                    self.walk(node.iif_stmt)


    def walk_FuncStmt(self, node):
        global builder
        global currScope
        global functionsDict
        debug_print('in FuncStmt')
        fn_name  = str(node.name)
        ret_type = None
        #return type
        if str(node.ret_type) in stdLibModule.typesMap: 
            ret_type = stdLibModule.typesMap[str(node.ret_type)]
        else:
            assert False, str(node.ret_type) + " is invalid return type"
        currScope = {'parent':currScope,
                     'scope':ir.Function(module, 
                                    ir.FunctionType(ret_type,[]), 
                                    name=fn_name)}
        functionsDict[fn_name] = currScope['scope']
        currBlock = currScope['scope'].append_basic_block()
        builder     = ir.IRBuilder(currBlock)
        self.walk(node.func_block)

    def walk_RetStmt(self,node):
        debug_print('in RetStmt')
        ret_val = self.walk(node.ret_val)
        return builder.ret(ret_val)

    def walk_ScopeBlock(self, node):
        debug_print('in ScopeBlock')
        return self.walk(node.statement)

    def walk_Comparison(self,node):
        debug_print('in Comparison')
        result = self.walk(node.lhs)
        if node.rhs is not None:
            comp_op = node.rhs[0].op
            rhs = self.walk(node.rhs[1])
            result = builder.icmp_signed(comp_op, result, rhs)
        return result 

    def walk_AssignStmt(self,node):
        debug_print('in AssignStmt')
        lhs = str(node.lhs)
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
        if node.bool:
            return Constant(IntType(1), int(0 if str(node.bool) == 'false' else 1))
        elif node.name:
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

parser = grammarParser(parseInfo=True, comments_re='\#.*\n')
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


with open(output_filename,'w') as f:
    f.write(str(module))

