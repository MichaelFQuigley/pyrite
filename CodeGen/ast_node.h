#include <array>
#include <iterator>

class AstNode {
    public:
        AstNode(){}

        virtual void codeGen() = 0;
};

class StmtsNode : public AstNode {
    private:
        AstNode* stmts;
    public:
        StmtsNode() 
            : AstNode(){ }

        void codeGen() override;
};

class VarDefNode: public AstNode {
    private:
        AstNode* stmts;
    public:
        VarDefNode() 
            : AstNode(){ }

        void codeGen() override;
};

class ExprOpNode: public AstNode {
    private:
        AstNode* stmts;
    public:
        ExprOpNode() 
            : AstNode(){ }

        void codeGen() override;
};

class FuncDefNode: public AstNode {
    private:
        AstNode* stmts;
    public:
        FuncDefNode() 
            : AstNode(){ }

        void codeGen() override;
};

class AtomOpNode: public AstNode {
    private:
        AstNode* stmts;
    public:
        AtomOpNode() 
            : AstNode(){ }

        void codeGen() override;
};

class LoopNode: public AstNode {
    private:
        AstNode* stmts;
    public:
        LoopNode() 
            : AstNode(){ }

        void codeGen() override;
};

class IfNode: public AstNode {
    private:
        AstNode* stmts;
    public:
        IfNode() 
            : AstNode(){ }

        void codeGen() override;
};

class BinOpNode: public AstNode {
    private:
        AstNode* stmts;
    public:
        BinOpNode() 
            : AstNode(){ }

        void codeGen() override;
};

