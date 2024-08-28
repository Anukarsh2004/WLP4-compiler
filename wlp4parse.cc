#include <iostream>
#include <deque>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <stack>

#include "wlp4data.h" 

using namespace std;

struct Rule {
    string lhs;
    vector<string> rhs;
};

struct ParseTreeNode {
    string value;
    vector<ParseTreeNode*> children;

    ParseTreeNode(const string& val) : value(val) {}
    ~ParseTreeNode() {
        for (auto child : children) {
            delete child;
        }
    }
};

void printParseTree(ParseTreeNode* node) {
    if (!node) return;
    cout << node->value << endl;
    for (auto child : node->children) {
        printParseTree(child);
    }
}

string ruletoString(Rule r) {
 string s = r.lhs;

 for (int i = 0; i < r.rhs.size(); i++) {
    s+=" ";
    s+=r.rhs[i];
 }

 return s;

}

void printState(const vector<string>& reductionSequence, const deque<pair<string, string>>& inputSequence) {
    for (const auto& symbol : reductionSequence) {
        cout << symbol << " ";
    }
    cout << ". ";
    for (const auto& token : inputSequence) {
        cout << token.first << " ";
    }
    cout << endl;
}

int main() {
    map<int, Rule> cfg;
    deque<pair<string, string>> inputSequence;
    vector<string> reductionSequence;
    map<int, map<string, int>> transitions;
    map<int, map<string, int>> reductions;
    stack<int> stateStack;
    stack<ParseTreeNode*> parseStack;
    int read = 0;


    istringstream wlp4Stream(WLP4_COMBINED);
    string line;

    while (getline(wlp4Stream, line) && line != ".TRANSITIONS") {
        if (line == ".CFG" || line.empty()) continue;
        istringstream iss(line);
        string lhs;
        iss >> lhs;
        int ruleNum = cfg.size();
        cfg[ruleNum] = {lhs, {}};
        string symbol;
        while (iss >> symbol) {
            cfg[ruleNum].rhs.push_back(symbol);
        }
    }

    while (getline(wlp4Stream, line) && line != ".REDUCTIONS") {
        if (line.empty()) continue;
        istringstream iss(line);
        int state;
        string symbol;
        int nextState;
        iss >> state >> symbol >> nextState;
        transitions[state][symbol] = nextState;
    }

    while (getline(wlp4Stream, line) && line != ".END") {
        if (line.empty()) continue;
        istringstream iss(line);
        int state, ruleNum;
        string lookahead;
        iss >> state >> ruleNum >> lookahead;
        reductions[state][lookahead] = ruleNum;
    }

    // Read input tokens
    inputSequence.push_back({"BOF","BOF"});
    while (getline(cin, line)) {
        istringstream iss(line);
        string kind, lexeme;
        iss >> kind >> lexeme;
        inputSequence.push_back({kind, lexeme});
    }
    inputSequence.push_back({"EOF","EOF"});

    stateStack.push(0);

    while (!inputSequence.empty()) {
        int currentState = stateStack.top();
        string nextKind = inputSequence.front().first;
        string lexeme = inputSequence.front().second;

        if (reductions[currentState].find(nextKind) != reductions[currentState].end()) {
            int ruleNum = reductions[currentState][nextKind];
            const Rule rule = cfg[ruleNum];
            int rhsSize = rule.rhs.size();
            ParseTreeNode* parentNode = new ParseTreeNode(ruletoString(rule));

            while (rhsSize != 0) {
                if (rule.rhs[0] != ".EMPTY") {
                    reductionSequence.pop_back();
                    stateStack.pop();
                    ParseTreeNode* childNode = parseStack.top();
                    parentNode->children.insert(parentNode->children.begin(), childNode);
                    parseStack.pop();   
                }
                rhsSize--;
            }
            
            parseStack.push(parentNode);
            reductionSequence.push_back(rule.lhs);
            currentState = stateStack.top();
            stateStack.push(transitions[currentState][rule.lhs]);
        }
        else if (transitions[currentState].find(nextKind) != transitions[currentState].end()) {
            read++;
            inputSequence.pop_front();
            reductionSequence.push_back(nextKind);
            stateStack.push(transitions[currentState][nextKind]);
            parseStack.push(new ParseTreeNode(nextKind + " " + lexeme));
        }
        else {
             while (!parseStack.empty()) {
               delete parseStack.top();
               parseStack.pop();
            }
            cerr << "ERROR at " << read << endl;
            return 1;
        }
    }
   
    int i = 3;
    ParseTreeNode* parentNode = new ParseTreeNode("start BOF procedures EOF");
   
    while (i != 0) {
                    reductionSequence.pop_back();
                    stateStack.pop();
                    ParseTreeNode* childNode = parseStack.top();
                    parentNode->children.insert(parentNode->children.begin(), childNode);
                    parseStack.pop();   
                    i --;
            }
    

    parseStack.push(parentNode);
    reductionSequence.push_back("start");
    printParseTree(parseStack.top());

    
    delete parseStack.top();

}
