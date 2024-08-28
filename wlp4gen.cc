#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <algorithm>

using namespace std;

struct Node
{
    string rule;
    string first;
    vector<string> tokens;
    vector<Node *> children;
    string type;

    Node(const string &r) : rule(r) {}
    ~Node()
    {
        for (Node *child : children)
        {
            delete child;
        }
    }
};

map<int, Node *> nodes;
int currentId = 0;
int ifcounter = 0;
int skipdeletecount = 0;

unordered_map<string, unordered_map<string, string>> procedureSymbolTables;
unordered_map<string, int> procedureoffsets;

void push(int x)
{
    cout << "sw $" << x << ", -4($30)" << endl;
    cout << "sub $30, $30, $4" << endl;
}

void pop(int x)
{
    cout << "add $30, $30, $4" << endl;
    cout << "lw $" << x << ", -4($30)" << endl;
}

Node *parseNode()
{
    string line;
    while (getline(cin, line))
    {
        Node *node = new Node(line);
        istringstream iss(line);
        string token;
        iss >> node->first;
        while (iss >> token)
        {
            node->tokens.push_back(token);
        }
        if (node->first == "NULL")
        {
            node->tokens.pop_back();
        }

        if (node->first == "expr" || node->first == "term" || node->first == "factor" || node->first == "NUM" || node->first == "NULL" || node->first == "lvalue")
        {
            node->type = node->tokens.back(); // Last token is the type annotation
            node->tokens.pop_back();          // pop type token
            node->tokens.pop_back();          // pop colon
        }

        if (node->first == "ID" && node->tokens.size() == 3)
        {
            node->type = node->tokens.back(); // Last token is the type annotation
            node->tokens.pop_back();          // pop type token
            node->tokens.pop_back();          // pop colon
        }

        nodes[currentId] = node;
        currentId++;

        for (int i = currentId - 2; i >= 0; i--)
        {

            vector<string> vec = nodes[i]->tokens;
            if (find(vec.begin(), vec.end(), node->first) != vec.end() && (nodes[i]->children.size() < vec.size()))
            {
                nodes[i]->children.push_back(node);
                break;
            }
        }
    }

    return nodes[0];
}

void argslistcounter(Node *node, int &counter)
{

    if (node->tokens.size() == 1)
    {
        counter++;
    }
    else
    {
        counter++;
        argslistcounter(node->children[2], counter);
    }
}

void generateMIPS(Node *node, string func)
{

    if (node->rule == "start BOF procedures EOF")
    {
        generateMIPS(node->children[1], func); // procedures
    }
    else if (node->rule == "procedures procedure procedures")
    {
        generateMIPS(node->children[1], func);
        generateMIPS(node->children[0], func);
    }
    else if (node->rule == "procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
    {
        string procedure = node->children[1]->tokens[0]; // ID
        procedureoffsets[procedure] = 0;
        int offset = 0;
        if (node->children[3]->tokens[0] == "paramlist")
        {
            argslistcounter(node->children[3]->children[0], offset);
        }
        offset *= 4;
        procedureoffsets[procedure] = offset;
        cout << "P" <<procedure << ":" << endl;
        cout << "sub $29, $30, $4" << endl;

        generateMIPS(node->children[3], procedure); // params
        generateMIPS(node->children[6], procedure); // dcls
        generateMIPS(node->children[7], procedure); // statements
        generateMIPS(node->children[9], procedure); // expr

        for (int i = 0; i < procedureSymbolTables[procedure].size(); i++)
        {
            cout << "add $30, $30, $4" << endl;
        }
        cout << "add $30 , $29 , $4" << endl;
        cout << "jr $31" << endl;
    }
    else if (node->first == "params")
    {
        if (node->tokens[0] == "paramlist")
        {
            generateMIPS(node->children[0], func);
        }
    }
    else if (node->first == "paramlist")
    {
        if (node->tokens.size() == 1)
        {
            generateMIPS(node->children[0], func);
        }
        else
        {
            generateMIPS(node->children[0], func);
            generateMIPS(node->children[2], func);
        }
    }
    else if (node->rule == "procedures main")
    {
        generateMIPS(node->children[0], func); // main
    }
    else if (node->rule == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE")
    {
        cout << "wain:" << endl;
        cout << "lis $4" << endl;
        cout << ".word 4" << endl;
        cout << "lis $11" << endl;
        cout << ".word 1" << endl;
        cout << ".import print" << endl;
        cout << ".import init" << endl;
        cout << ".import new" << endl;
        cout << ".import delete" << endl;
        cout << "sub $29, $30, $4" << endl;
        push(1);
        push(2);
        push(31);
        if (node->children[3]->children[1]->type == "int")
        {
            cout << "add $2, $0, $0" << endl;
        }

        cout << "lis $5" << endl;
        cout << ".word init" << endl;
        cout << "jalr $5" << endl;


        pop(31);

        generateMIPS(node->children[3], func); // dcl1

        generateMIPS(node->children[5], func); // dcl2

        generateMIPS(node->children[8], func); // dcls

        generateMIPS(node->children[9], func); // statements

        generateMIPS(node->children[11], func); // expr

        for (int i = 0; i < procedureSymbolTables[func].size(); i++)
        {
            cout << "add $30, $30, $4" << endl;
        }
        cout << "jr $31" << endl;
    }
    else if (node->first == "dcls")
    {
        if (node->rule == "dcls dcls dcl BECOMES NUM SEMI")
        {
            generateMIPS(node->children[0], func);     // dcls
            generateMIPS(node->children[1], func);     // dcl;
            string num = node->children[3]->tokens[0]; // NUM
            cout << "lis $3" << endl;
            cout << ".word " << num << endl;
            push(3);
        }
        else if (node->rule == "dcls dcls dcl BECOMES NULL SEMI")
        {
            generateMIPS(node->children[0], func);
            generateMIPS(node->children[1], func);
            cout << "add $3, $11, $0" << endl;
            push(3);
        }
    }
    else if (node->first == "dcl")
    {
        if (procedureSymbolTables[func].find(node->children[1]->tokens[0]) == procedureSymbolTables[func].end())
        {
            int offset = procedureoffsets[func];
            procedureSymbolTables[func][node->children[1]->tokens[0]] = to_string(offset);
            offset -= 4;
            procedureoffsets[func] = offset;
        }
    }
    else if (node->first == "statements")
    {
        if (node->tokens[0] == "statements")
        {
            generateMIPS(node->children[0], func); // statements
            generateMIPS(node->children[1], func); // statement
        }
    }
    else if (node->first == "statement")
    {
        if (node->tokens[0] == "IF")
        {
            // IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
            int counter = ifcounter;
            ifcounter++;
            generateMIPS(node->children[2], func); // test
            cout << "beq $3, $0, else" << counter << endl;
            generateMIPS(node->children[5], func); // statements (if block)
            cout << "beq $0 , $0 , endif" << counter << endl;
            cout << "else" << counter << ":" << endl;
            generateMIPS(node->children[9], func); // statements (else block)
            cout << "endif" << counter << ":" << endl;
        }
        else if (node->tokens[0] == "WHILE")
        {
            // WHILE LPAREN test RPAREN LBRACE statements RBRACE
            int counter = ifcounter;
            ifcounter++;
            cout << "loop" << counter << ":" << endl;
            generateMIPS(node->children[2], func); // test
            cout << "beq $3, $0, endwhile" << counter << endl;
            generateMIPS(node->children[5], func); // statements (while body)
            cout << "beq $0 , $0 , loop" << counter << endl;
            cout << "endwhile" << counter << ":" << endl;
        }
        else if (node->tokens[0] == "PUTCHAR")
        {
            // PUTCHAR LPAREN expr RPAREN SEMI
            generateMIPS(node->children[2], func); // expr
            cout << "lis $5" << endl;
            cout << ".word 0xffff000c" << endl;
            cout << "sw $3, 0($5)" << endl;
        }
        else if (node->tokens[0] == "PRINTLN")
        {
            // PRINTLN LPAREN expr RPAREN SEMI
            push(1);
            generateMIPS(node->children[2], func); // expr
            cout << "add $1, $3, $0" << endl;
            push(31);
            cout << "lis $5" << endl;
            cout << ".word print" << endl;
            cout << "jalr $5" << endl;
            pop(31);
            pop(1);
        }
        else if (node->tokens[0] == "DELETE")
        { // statement → DELETE LBRACK RBRACK expr SEMI
            generateMIPS(node->children[3], func);
            int counter = skipdeletecount;
            skipdeletecount++;
            cout << "beq $3, $11 ,skipDelete" << counter << endl;
            cout << "add $1, $3, $0" << endl;
            cout << "lis $5" << endl;
            cout << ".word delete" << endl;
            push(31);
            cout << "jalr $5" << endl;
            pop(31);
            cout << "skipDelete"<<counter<<":" << endl;
        }
        else
        {                                          // statement → lvalue BECOMES expr SEMI
            generateMIPS(node->children[2], func); // expr
            Node *lvalue = node->children[0];
            while (lvalue->tokens.size() == 3)
            {
                lvalue = lvalue->children[1];
            }

            if (lvalue->tokens[0] == "ID")
            {                                                                                                        // lvalue ID
                cout << "sw $3, " << procedureSymbolTables[func][lvalue->children[0]->tokens[0]] << "($29)" << endl; // store result
            }
            else
            { // lvalue STAR FACTOR

                push(3);
                generateMIPS(lvalue->children[1], func);
                pop(5);
                cout << "sw $5, 0($3)" << endl;
            }
        }
    }
    else if (node->first == "expr")
    {
        if (node->tokens.size() == 3) // expr PLUS/MINUS term
        {
            if (node->children[0]->type == "int*" && node->children[2]->type == "int")
            {
                generateMIPS(node->children[0], func); // expr
                push(3);
                generateMIPS(node->children[2], func); // term
                cout << "mult $3 , $4" << endl;
                cout << "mflo $3" << endl;
                pop(5);

                string op = node->tokens[1];

                if (op == "PLUS")
                {
                    cout << "add $3, $5, $3" << endl;
                }
                else if (op == "MINUS")
                {
                    cout << "sub $3, $5, $3" << endl;
                }
            }
            else if (node->children[0]->type == "int" && node->children[2]->type == "int*")
            {
                generateMIPS(node->children[0], func); // expr
                cout << "mult $3 , $4" << endl;
                cout << "mflo $3" << endl;
                push(3);
                generateMIPS(node->children[2], func); // term
                pop(5);
                string op = node->tokens[1];

                if (op == "PLUS")
                {
                    cout << "add $3, $5, $3" << endl;
                }
                else if (op == "MINUS")
                {
                    cout << "sub $3, $5, $3" << endl;
                }
            }
            else if (node->children[0]->type == "int" && node->children[2]->type == "int")
            {
                generateMIPS(node->children[0], func); // expr
                push(3);
                generateMIPS(node->children[2], func); // term
                pop(5);

                string op = node->tokens[1];
                if (op == "PLUS")
                {
                    cout << "add $3, $5, $3" << endl;
                }
                else if (op == "MINUS")
                {
                    cout << "sub $3, $5, $3" << endl;
                }
            }
            else if (node->children[0]->type == "int*" && node->children[2]->type == "int*")
            {

                generateMIPS(node->children[0], func); // expr
                push(3);
                generateMIPS(node->children[2], func); // term
                pop(5);
                cout << "sub $3 , $5 , $3" << endl;
                cout << "div $3 , $4" << endl;
                cout << "mflo $3" << endl;
            }
        }
        else
        {
            generateMIPS(node->children[0], func); // term
        }
    }
    else if (node->first == "term")
    {
        if (node->tokens.size() == 3)
        {
            generateMIPS(node->children[0], func); // term
            push(3);
            generateMIPS(node->children[2], func); // factor
            pop(5);

            string op = node->tokens[1];

            if (op == "STAR")
            {
                cout << "mult $3, $5" << endl;
                cout << "mflo $3" << endl;
            }
            else if (op == "SLASH")
            {
                cout << "div $5, $3" << endl;
                cout << "mflo $3" << endl;
            }
            else if (op == "PCT")
            {
                cout << "div $5, $3" << endl;
                cout << "mfhi $3" << endl;
            }
        }
        else
        {
            generateMIPS(node->children[0], func); // factor
        }
    }
    else if (node->first == "factor")
    {
        if (node->tokens[0] == "NUM")
        {
            string num = node->children[0]->tokens[0];
            cout << "lis $3" << endl;
            cout << ".word " << num << endl;
        }
        else if (node->tokens[0] == "ID")
        {
            if (node->tokens.size() == 1)
            { // factor ID
                cout << "lw $3, " << procedureSymbolTables[func][node->children[0]->tokens[0]] << "($29)" << endl;
            }
            else if (node->tokens.size() == 3)
            { // factor ID LPAREN RPAREN
                push(29);
                push(31);
                cout << "lis $5" << endl;
                cout << ".word " << "P" << node->children[0]->tokens[0] << endl;
                cout << "jalr $5" << endl;
                pop(31);
                pop(29);
            }
            else
            { // factor ID LPAREN arglist RPAREN
                push(29);
                push(31);
                int counter = 0;
                argslistcounter(node->children[2], counter);
                procedureoffsets[node->children[0]->tokens[0]] += 4 * counter;
                generateMIPS(node->children[2], func);
                cout << "lis $5" << endl;
                cout << ".word " << "P" << node->children[0]->tokens[0] << endl;
                cout << "jalr $5" << endl;
                for (int i = 0; i < counter; i++)
                {
                    pop(5);
                }
                pop(31);
                pop(29);
            }
        }
        else if (node->tokens[0] == "LPAREN")
        {
            generateMIPS(node->children[1], func);
        }
        else if (node->tokens[0] == "GETCHAR")
        {
            cout << "lis $5" << endl;
            cout << ".word 0xffff0004" << endl;
            cout << "lw $3, 0($5)" << endl;
        }
        else if (node->tokens[0] == "NULL")
        {
            cout << "add $3, $0, $11" << endl;
        }
        else if (node->tokens[0] == "STAR") // factor star factor
        {
            generateMIPS(node->children[1], func);
            cout << "lw $3, 0($3)" << endl;
        }

        else if (node->tokens[0] == "AMP") // factor AMP lvalue
        {
            Node *lvalue = node->children[1];
            while (lvalue->tokens.size() == 3)
            {
                lvalue = lvalue->children[1];
            }

            if (lvalue->tokens[0] == "STAR") // lvalue STAR factor
            {
                generateMIPS(lvalue->children[1], func);
            }
            else
            { // lvalue ID
                string idoffset = procedureSymbolTables[func][lvalue->children[0]->tokens[0]];
                cout << "lis $3" << endl;
                cout << ".word " << idoffset << endl;
                cout << "add $3, $29, $3" << endl;
            }
        }
        else if (node->tokens[0] == "NEW")
        { // factor → NEW INT LBRACK expr RBRACK
            generateMIPS(node->children[3], func);
            cout << "add $1, $3, $0" << endl;
            cout << "lis $5" << endl;
            cout << ".word new" << endl;
            push(31);
            cout << "jalr $5" << endl;
            pop(31);
            cout << "bne $3, $0, 1" << endl;
            cout << "add $3, $11, $0" << endl;
        }
    }
    else if (node->first == "arglist")
    {
        if (node->tokens.size() == 1) // arglist expr
        {
            generateMIPS(node->children[0], func);
            push(3);
        }
        else
        {
            generateMIPS(node->children[0], func);
            push(3);
            generateMIPS(node->children[2], func);
        }
    }
    else if (node->first == "test")
    {
        // expr comparison expr
        generateMIPS(node->children[0], func); // expr1
        push(3);                               // save result of expr1
        generateMIPS(node->children[2], func); // expr2
        pop(5);                                // restore result of expr1
        bool pointer = node->children[0]->type == "int*";

        string op = node->tokens[1];
        if (op == "EQ")
        {
            if (pointer)
            {
                cout << "sltu $6 , $3 , $5" << endl;
                cout << "sltu $7 , $5 , $3" << endl;
                cout << "add $3 , $6 , $7" << endl;
                cout << "sub $3 , $11 , $3" << endl;
            }
            else
            {
                cout << "slt $6 , $3 , $5" << endl;
                cout << "slt $7 , $5 , $3" << endl;
                cout << "add $3 , $6 , $7" << endl;
                cout << "sub $3 , $11 , $3" << endl;
            }
        }
        else if (op == "NE")
        {
            if (pointer)
            {
                cout << "sltu $6 , $3 , $5" << endl;
                cout << "sltu $7 , $5 , $3" << endl;
                cout << "add $3 , $6 , $7" << endl;
            }
            else
            {
                cout << "slt $6 , $3 , $5" << endl;
                cout << "slt $7 , $5 , $3" << endl;
                cout << "add $3 , $6 , $7" << endl;
            }
        }
        else if (op == "LT")
        {
            if (pointer)
            {
                cout << "sltu $3, $5, $3" << endl;
            }
            else
            {
                cout << "slt $3, $5, $3" << endl;
            }
        }
        else if (op == "LE")
        {
            if (pointer)
            {
                cout << "sltu $3, $3, $5" << endl;
                cout << "sub $3 , $11 , $3" << endl;
            }
            else
            {
                cout << "slt $3, $3, $5" << endl;
                cout << "sub $3 , $11 , $3" << endl;
            }
        }
        else if (op == "GE")
        {
            if (pointer)
            {
                cout << "sltu $3, $5, $3" << endl;
                cout << "sub $3 , $11 , $3" << endl;
            }
            else
            {
                cout << "slt $3, $5, $3" << endl;
                cout << "sub $3 , $11 , $3" << endl;
            }
        }
        else if (op == "GT")
        {
            if (pointer)
            {
                cout << "sltu $3, $3, $5" << endl;
            }
            else
            {
                cout << "slt $3, $3, $5" << endl;
            }
        }
    }
}

int main()
{
    Node *root = parseNode();
    generateMIPS(root, "wain");
    delete root;
}
