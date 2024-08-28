#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <unordered_map>

using namespace std;

struct TreeNode
{
    string value;
    string first;
    vector<TreeNode *> children;
    vector<string> childtype;
    string type = "";
    bool checked = false;

    TreeNode(string v, string f, vector<string> ct) : value(v), first(f), childtype(ct) {}
    ~TreeNode()
    {
        for (auto child : children)
        {
            delete child;
        }
    }
};

struct Procedure
{
    string name;
    vector<string> paramTypes;
};

unordered_map<string, unordered_map<string, string>> procedureSymbolTables;
unordered_map<string, Procedure> procedures;
map<int, TreeNode *> nodes;
int currentId = 0;

TreeNode *parseInput()
{
    string line;

    while (getline(cin, line))
    {
        istringstream iss(line);
        string value = line;
        string first, ctype;
        iss >> first;
        vector<string> ct = {};
        while (iss >> ctype)
        {
            ct.push_back(ctype);
        }

        if (first == "NULL")
        {
            ct.pop_back();
        }

        TreeNode *node = new TreeNode(value, first, ct);
        nodes[currentId] = node;
        currentId++;

        if (currentId == 1)
        {
            continue;
        }

        for (int i = currentId - 2; i >= 0; i--)
        {
            vector<string> vec = nodes[i]->childtype;
            if (find(vec.begin(), vec.end(), first) != vec.end() && (nodes[i]->children.size() < vec.size()))
            {
                nodes[i]->children.push_back(node);
                break;
            }
        }
    }

    return nodes[0];
}

void dclTypeMatchCheck(TreeNode *dcls)
{
    string vartype = dcls->children[1]->children[1]->type;
    string valuetype = dcls->childtype[3];

    if (vartype == "int" && valuetype == "NULL")
    {
        cerr << "ERROR mismatch assignment" << endl;
    }
    if (vartype == "int*" && valuetype != "NULL")
    {
        cerr << "ERROR mismatch assignment" << endl;
    }
}

void statementTypeCheck(TreeNode *stmt)
{
    string lefttype = stmt->children[0]->type;
    string righttype = stmt->children[2]->type;

    if (lefttype != righttype)
    {
        cerr << "ERROR mismatch types" << endl;
    }
}




void returnTypeCheck(TreeNode *node)
{
    string type = node->children[9]->type;

    if (type != "int")
    {
        cerr << "ERROR return type not int" << endl;
    }
}

void checkSemanticErrors(TreeNode *root)
{
    TreeNode *main = nullptr;
    TreeNode *expr = nullptr;

    for (int i = 0; i < currentId; i++)
    {
        auto child = nodes[i];

        if (child->first == "main")
        {
            main = child;
            expr = child->children[11];
        }

        if (child->first == "procedure")
        {
            returnTypeCheck(child);
        }

        if (child->first == "dcls" && child->childtype.size() == 5)
        {
            dclTypeMatchCheck(child);
        }

        if (child->first == "statement" && child->childtype.size() == 4) {
            statementTypeCheck(child);
        }

        if (child->first == "test") {
            statementTypeCheck(child);
        }

        if (child->first == "statement" && child->childtype.size() == 5) {
            if(child->childtype[0] == "DELETE") {
                if (child->children[3]->type != "int*") {
                    cerr << "ERROR delete has wrong type" << endl; 
                }
            } else {
                if (child->children[2]->type != "int") {
                    cerr << "ERROR println/putchar has wrong type" << endl; 
                }
            }
        }

    }

    if (main->children[5]->children[1]->type != "int")
    {
        cerr << "ERROR second paramter of wain is not int" << endl;
    }
    if (expr && expr->type != "int")
    {
        cerr << "ERROR return type of wain not int" << endl;
    }
}

void annotateTree(TreeNode *root, const string currentProc);
void getParamTypes(TreeNode *pl, vector<string> &paramTypes, const string proc)
{
    if (pl->childtype.size() == 1)
    {
        annotateTree(pl->children[0], proc);
        paramTypes.push_back(pl->children[0]->children[1]->type);
    }
    else
    {
        annotateTree(pl->children[0], proc);
        paramTypes.push_back(pl->children[0]->children[1]->type);
        getParamTypes(pl->children[2], paramTypes, proc);
    }
}

void getArglist(TreeNode *al, vector<string> &argTypes, const string proc)
{
    if (al->childtype.size() == 1)
    {
        annotateTree(al->children[0], proc);
        argTypes.push_back(al->children[0]->type);
    }
    else
    {
        annotateTree(al->children[0], proc);
        argTypes.push_back(al->children[0]->type);
        getArglist(al->children[2], argTypes, proc);
    }
}

void annotateTree(TreeNode *root, const string currentProc)
{
    if (!(root->checked))
    {
        if (root->first == "procedure")
        {
            string procName = root->children[1]->childtype[0];
            if (procedures.find(procName) != procedures.end())
            {
                cerr << "ERROR duplicate declaration of procedure" << endl;
            }
            else
            {
                vector<string> paramTypes;
                TreeNode *params = root->children[3];
                if (params->childtype[0] == "paramlist")
                {
                    getParamTypes(params->children[0], paramTypes, currentProc);
                }

                procedures[procName] = {procName, paramTypes};
                root->checked = true;
            }
        }
        else if (root->first == "dcl")
        {
            string type = root->children[0]->childtype.size() == 1 ? "int" : "int*";
            string varName = root->children[1]->childtype[0];
            if (procedureSymbolTables[currentProc].find(varName) != procedureSymbolTables[currentProc].end())
            {
                cerr << "ERROR duplicate variable declaration in "<< currentProc << endl;
            }
            else
            {
                procedureSymbolTables[currentProc][varName] = type;
            }
            root->children[1]->type = type;
            root->checked = true;
        }
        else if (root->first == "factor" && root->childtype.size() == 1 && root->childtype[0] == "ID")
        {
            string varName = root->children[0]->childtype[0];
            if (procedureSymbolTables[currentProc].find(varName) == procedureSymbolTables[currentProc].end())
            {
                cerr << "ERROR undeclared variable in procedure" << endl;
            }
            else
            {
                root->type = procedureSymbolTables[currentProc][varName];
                root->children[0]->type = procedureSymbolTables[currentProc][varName];
            }
            root->checked = true;
        }
        else if ((root->first == "expr" || root->first == "term") && root->children.size() == 3)
        {
            TreeNode *left = root->children[0];
            TreeNode *right = root->children[2];
            annotateTree(left, currentProc);
            annotateTree(right, currentProc);

            string op = root->childtype[1];
            if (op == "PLUS")
            {
                if (left->type == "int" && right->type == "int")
                {
                    root->type = "int";
                }
                else
                {
                    root->type = "int*";
                }
            }
            if (op == "MINUS")
            {
                if (left->type == right->type)
                {
                    root->type = "int";
                }
                else
                {
                    root->type = "int*";
                }
            }

            else if (op == "STAR" || op == "SLASH" || op == "PCT")
            {
                if (left->type != "int" || right->type != "int")
                {
                    cerr << "ERROR types aren't int for */%" << endl;
                }
                else
                {
                    root->type = "int";
                }
            }
            root->checked = true;
        }
        else if (root->first == "factor" && root->childtype.size() == 2)
        {
            TreeNode *operand = root->children[1];
            annotateTree(operand, currentProc);

            string op = root->childtype[0];
            if (op == "AMP")
            {
                if (operand->type != "int")
                {
                    cerr << "ERROR address of non-int" << endl;
                }
                else
                {
                    root->type = "int*";
                }
            }
            else if (op == "STAR")
            {
                if (operand->type != "int*")
                {
                    cerr << "ERROR derefrencing non-pointer" << endl;
                }
                else
                {
                    root->type = "int";
                }
            }
            root->checked = true;
        }
        else if (root->first == "factor" && root->childtype.size() == 5)
        {
            TreeNode *operand = root->children[3];
            annotateTree(operand, currentProc);

            if (operand->type != "int")
            {
                cerr << "ERROR wrong type in NEW" << endl;
            }
            else
            {
                root->type = "int*";
            }
            root->checked = true;
        }
        else if (root->first == "factor" && root->childtype.size() == 4)
        {
            string procName = root->children[0]->childtype[0];
            if (procedures.find(procName) == procedures.end())
            {
                cerr << "ERROR undeclared procedure" << endl;
            }
            else
            {
                vector<string> argTypes;
                getArglist(root->children[2], argTypes, currentProc);
                vector<string> paramTypes = procedures[procName].paramTypes;
                if (argTypes == paramTypes)
                {
                    root->type = "int";
                }
                else
                {
                    cerr << "ERROR47" << endl;
                }
            }
            root->checked = true;
        }
        else if (root->first == "factor" && root->childtype.size() == 3)
        {
            if (root->childtype[0] == "LPAREN")
            {
                TreeNode *operand = root->children[1];
                annotateTree(operand, currentProc);

                root->type = operand->type;
            }
            else
            {
                string procName = root->children[0]->childtype[0];
                if (procedures.find(procName) == procedures.end() && procName != "getchar")
                {
                    cerr << "ERROR undeclared procedure" << endl;
                }
                else
                {
                    vector<string> paramTypes = procedures[procName].paramTypes;
                    if (paramTypes.empty())
                    {
                        root->type = "int";
                    }
                    else
                    {
                        cerr << "ERROR49" << endl;
                    }
                }
                root->checked = true;
            }
        }
        else if (root->first == "expr" || root->first == "term" || root->first == "factor")
        {
            TreeNode *operand = root->children[0];
            annotateTree(operand, currentProc);
            root->type = operand->type;
            root->checked = true;
        }
        else if (root->first == "lvalue")
        {
            if (root->childtype.size() == 1)
            {
                string varName = root->children[0]->childtype[0];
                if (procedureSymbolTables[currentProc].find(varName) == procedureSymbolTables[currentProc].end())
                {
                    cerr << "ERROR undeclared variable" << endl;
                }
                else
                {

                    root->type = procedureSymbolTables[currentProc][varName];
                    root->children[0]->type = procedureSymbolTables[currentProc][varName];
                }
            }
            else if (root->childtype.size() == 2)
            {
                TreeNode *factor = root->children[1];
                annotateTree(factor, currentProc);
                if (factor->type != "int*")
                {
                    cerr << "ERROR" << endl;
                }
                else
                {
                    root->type = "int";
                }
            }
            else if (root->childtype.size() == 3)
            {
                TreeNode *lvalue = root->children[1];
                annotateTree(lvalue, currentProc);
                root->type = lvalue->type;
            }
            root->checked = true;
        }
        else if (root->first == "NULL")
        {
            root->type = "int*";
            root->checked = true;
        }
        else if (root->first == "NUM")
        {
            root->type = "int";
            root->checked = true;
        }
    }

    for (auto child : root->children)
    {
        if (child->first == "procedure")
        {
            annotateTree(child, child->children[1]->childtype[0]);
        }
        else if (child->first == "main")
        {
            annotateTree(child, "wain");
        }
        else
        {
            annotateTree(child, currentProc);
        }
    }
}

void printParseTree(TreeNode *node)
{
    if (!node)
        return;
    if (node->type != "")
    {
        cout << node->value << " : " << node->type << endl;
    }
    else
    {
        cout << node->value << endl;
    }

    for (auto child : node->children)
    {
        printParseTree(child);
    }
}

int main()
{
    TreeNode *root = parseInput();
    annotateTree(root, "");
    checkSemanticErrors(root);
    printParseTree(root);
    delete root;
}
