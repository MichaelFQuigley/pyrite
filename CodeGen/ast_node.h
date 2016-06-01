#include <array>
#include <iterator>

class AstNode {
    private:
        AstNode** children;
        int children_length;

    public:
        AstNode(AstNode** children, int children_length)
        {
            this->children        = children;
            this->children_length = children_length;
        }

        virtual void codeGen() = 0;

        int getNumChildren() { return children_length; }

        AstNode* getChild(int index) { 
            if(index < children_length) { 
                return children[index];
            } 
            return NULL; 
        }

        void setChildren(AstNode** children) { this->children = children; }
};

class StmtsNode : public AstNode {
    private:
        AstNode* stmts;
    public:
        StmtsNode(AstNode** children, int children_length) 
            : AstNode(children, children_length){ }

        void codeGen() override;
};
