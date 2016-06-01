#include "ast_node.h"
#include <json/json.h>

class AstWalker
{
    public:
        void codeGen_initial(Json::Value json_node);
        void codeGen_stmts(Json::Value json_node);
        Json::Value generateFromJson(std::string json_string);
};
