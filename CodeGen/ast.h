#include "ast_node.h"
#include <json/json.h>

class Ast
{
    private:
        Json::Value root;
    public:
        bool generateFromJson(std::string json_string);
};
