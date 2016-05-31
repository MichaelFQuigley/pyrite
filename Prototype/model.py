import grako
from myParse import *
from grako.model import ModelBuilderSemantics, Node, NodeWalker
from llvmlite import ir
from llvmlite import binding
from llvmlite.ir import Constant, IntType, DoubleType, ArrayType
from types_util import *
from parse_util import *
from stdlib import *
from scope_helper import ScopeHelper

output_filename = 'foo.ll'

llvm_gen = []

module      = ir.Module(name=output_filename, triple=binding.get_default_triple())
#currScope = {'parent':None, 'scope':None}
globalBlock = None 
builder     = None 
scopeHelper = ScopeHelper()

#format of currLambdas is {'name': [Function, lambda_node, fn_args, named_val_tup]}
#named_val_tup is of the format [(name, value, size), ...]
currLambdas = {}

stdLibModule = StdLib('stdlib/stdlib.ll', module)

functionsDict = {
        'print':
            ir.Function(module, 
                        ir.FunctionType(IntType(32), 
                                (ir.PointerType(IntType(8)),),
                                var_arg=True),
                        'printf'),
            'llvm.init.trampoline':ir.Function(module,
                                                ir.FunctionType(ir.VoidType(),
                                                    (ir.PointerType(IntType(8)),ir.PointerType(IntType(8)),ir.PointerType(IntType(8)),)),
                                    'llvm.init.trampoline'),
            'llvm.adjust.trampoline':ir.Function(module,
                                                ir.FunctionType(ir.PointerType(ir.IntType(8)),
                                                    (ir.PointerType(IntType(8)),)),
                                    'llvm.adjust.trampoline')
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

    def resolveTypeFromTypeFormat(self, type_format_node):
        if type_format_node.simple_arg:
            return stdLibModule.getType(type_format_node.simple_arg)
        elif type_format_node.func_arg:
            ret_type = self.resolveTypeFromTypeFormat(type_format_node.ret_type)
            args_list = self.walkTypedArgs(type_format_node.arguments, use_name=False)
            return ir.PointerType(ir.FunctionType(ret_type, args_list))
        else:
            assert False, 'type ' + str(type_format_node) + ' not implemented'

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
        scopeHelper.pushScope()

        if node.first_stmt: self.walk(node.first_stmt)
        
        top_of_loop    = builder.append_basic_block(module.get_unique_name('loop_top'))
        body_of_loop   = builder.append_basic_block(module.get_unique_name('loop_body'))
        bottom_of_loop = builder.append_basic_block(module.get_unique_name('loop_bottom'))
        
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
        scopeHelper.popScope()

    def walk_IfStmt(self,node):
        debug_print('in IfStmt')

        scopeHelper.pushScope()
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
        scopeHelper.popScope()

    def walkTypedArgs(self, typed_args, use_name=True):
        #returns list of tuples for typed args
        #[(name, type), ...]
        args_list = []
        if typed_args is None:
            return args_list
        if use_name:
            args_list.append((str(typed_args[0]), 
                                self.resolveTypeFromTypeFormat(typed_args[2])))
            if len(typed_args) == 3:
                return args_list
            args_rest = typed_args[3]
            for arg in args_rest:
                args_list.append((str(arg[1]),
                                self.resolveTypeFromTypeFormat(arg[3])))
        #typed_args_no_name case
        else:
            args_list.append(self.resolveTypeFromTypeFormat(typed_args[0]))
            if len(typed_args) == 1:
                return args_list
            args_rest = typed_args[1]
            for arg in args_rest:
                args_list.append(self.resolveTypeFromTypeFormat(arg[1]))
        return args_list

    def walk_FuncStmt(self, node):
        global builder
        global functionsDict
        debug_print('in FuncStmt')
        fn_name  = str(node.name)
        fn_args  = self.walkTypedArgs(node.arguments)
        #return type
        ret_type = self.resolveTypeFromTypeFormat(node.ret_type)

        func = ir.Function(module, 
                            ir.FunctionType(ret_type,
                            [arg[1] for arg in fn_args]), 
                            name=fn_name)
        scopeHelper.pushScope(isFunctionScope=True)
        #adding function arguments to scope
        for i in range(len(fn_args)):
            scopeHelper.setNamedVal(fn_args[i][0], func.args[i])

        functionsDict[fn_name] = func
        currBlock = func.append_basic_block()
        builder     = ir.IRBuilder(currBlock)
        self.walk(node.func_block)
        scopeHelper.popScope()
        self.processLambdas()

    def processLambdas(self):
        if len(currLambdas) == 0:
            return
        global builder
        while len(currLambdas.keys()) > 0:
            lamKey = currLambdas.keys()[0]
            lamAttrs = currLambdas[lamKey]
            del currLambdas[lamKey]
            lamNode = lamAttrs[1]
            ret_type = self.resolveTypeFromTypeFormat(lamNode.ret_type) 
            func     = lamAttrs[0]
            fn_args  = lamAttrs[2]
            scopeHelper.pushScope(isFunctionScope=True)
            
            currBlock = func.append_basic_block()
            builder     = ir.IRBuilder(currBlock)
            named_vals = lamAttrs[3]
            currInd = 0
            env_array = builder.bitcast(func.args[0], 
                                        ir.PointerType(ir.ArrayType(IntType(8),reduce(lambda x,y: x + y[2],named_vals,0))))
            for named_val_name, named_val_value, named_val_size in named_vals:
                arr_ind = builder.gep(env_array, [Constant(IntType(32), 0), Constant(IntType(32), currInd)], inbounds=True)
                val_ptr = builder.bitcast(arr_ind, ir.PointerType(named_val_value.type))
                scopeHelper.setNamedVal(named_val_name, val_ptr)
                currInd += named_val_size
            #add function arguments to scope    
            for i in range(len(fn_args)):
                scopeHelper.setNamedVal(fn_args[i][0], func.args[i])

            self.walk(lamNode.lambda_block)
            scopeHelper.popScope()
            self.processLambdas()

    def walk_LambdaStmt(self, node):
        global functionsDict
        debug_print('in LambdaStmt')
        ret_type = self.resolveTypeFromTypeFormat(node.ret_type) 
        fn_name  = module.get_unique_name('')
        #first arg for lambda is nest parameter
        fn_args  = [(module.get_unique_name(''), ir.PointerType(IntType(8)))] + self.walkTypedArgs(node.arguments)
        func = ir.Function(module, 
                            ir.FunctionType(ret_type,
                            [arg[1] for arg in fn_args]), 
                            name=module.get_unique_name(fn_name))
        func.args[0].add_attribute('nest')

        #format is [(name, value, size), ...]
        closure_data_vars = []
        for val in scopeHelper.getNamedVals():
            size = None
            if type(val[1]) == ir.Argument:
                closure_data_vars += [(val[0], val[1], get_type_size(val[1].type))]
            else:
                value = builder.load(val[1])
                closure_data_vars += [(val[0], value, get_type_size(value.type))]
        
        closure_data_size = reduce(lambda x,y: x + y[2], closure_data_vars, 0)

        closure_data = builder.alloca(ir.ArrayType(ir.IntType(8), closure_data_size))
        currInd = 0
        for value in closure_data_vars:
            arr_ind = builder.gep(closure_data, [Constant(IntType(32), 0), Constant(IntType(32), currInd)], inbounds=True)
            arr_ind = builder.bitcast(arr_ind, ir.PointerType(value[1].type))
            builder.store(value[1], arr_ind)
            currInd += value[2] 

        env = builder.bitcast(closure_data, ir.PointerType(IntType(8)))

        tramp = builder.alloca(ir.ArrayType(ir.IntType(8), 72), align=8)
        tramp1 = builder.gep(tramp, [Constant(IntType(32),0), Constant(IntType(32),0)])
        builder.call(functionsDict['llvm.init.trampoline'],[tramp1, func.bitcast(ir.PointerType(IntType(8))) , env])
        temp_func = builder.call(functionsDict['llvm.adjust.trampoline'],[tramp1])
        sanitizedArgs = [arg[1] for arg in fn_args[1:]] if len(fn_args) > 0 else []
        #args with extra nest param passed in for now
        currLambdas[fn_name] = [func, node, fn_args, closure_data_vars]
        result_func = builder.bitcast(temp_func, ir.PointerType(ir.FunctionType(ret_type, sanitizedArgs)))
        return result_func

    def walk_RetStmt(self,node):
        debug_print('in RetStmt')
        ret_val = self.walk(node.ret_val)
        if ret_val is not None:
            return builder.ret(ret_val)
        else:
            return builder.ret_void()

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
        rhs = self.walk(node.rhs)
        var_type = rhs.type
        #if type is list, then left side is declaration
        if type(node.lhs) == list:
            var_name = str(node.lhs[1])
            if len(node.lhs) == 4:
                assert scopeHelper.getNamedVal(var_name,walkScopes=False) is None, "Variable " + var_name + " already declared"
                var_type  = self.resolveTypeFromTypeFormat(node.lhs[3])
            scopeHelper.setNamedVal(var_name,
                                    builder.alloca(var_type, name=module.get_unique_name(var_name)))
        else:
            var_name = str(node.lhs)
            assert scopeHelper.getNamedVal(var_name,walkScopes=True) is not None, "Variable " + var_name + " is not declared"
        return builder.store(rhs,scopeHelper.getNamedVal(var_name,walkScopes=True))

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
                #is it a function argument?
                atom_var = scopeHelper.getNamedVal(str(node.at.name), walkScopes=True)
                if type(atom_var) == ir.Argument:
                    return atom_var
                else:
                    return builder.load(atom_var, str(node.at.name))
            return self.walk(node.at)
        elif node.fcall:
            fname = str(list(node.fcall)[0])
            func = None
            if fname in functionsDict:
                func = functionsDict[fname]
            elif scopeHelper.getNamedVal(fname, walkScopes=True) is not None:
                named_func = scopeHelper.getNamedVal(fname, walkScopes=True)
                if type(named_func) == ir.Argument:
                    func = named_func
                else:
                    func = builder.load(scopeHelper.getNamedVal(fname, walkScopes=True), fname)
            else:
                assert False, "Undeclared function: " + str(fname)
            return builder.call(func, 
                    [] if len(node.fcall) == 3 
                    else self.walk(node.fcall[2]))
        elif node.sub_expr:
            return self.walk(node.sub_expr)

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
    f.write('declare void @llvm.init.trampoline(i8* , i8*, i8* )\n')
    f.write('declare i8* @llvm.adjust.trampoline( i8* )\n')
    f.write(str(module))
