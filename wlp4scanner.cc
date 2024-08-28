#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

const std::unordered_set<std::string> KEYWORDS = {
    "wain", "int", "if", "else", "while", "println", "putchar", "getchar",
    "return", "NULL", "new", "delete"
};
const std::unordered_map<std::string, std::string> KEYWORD_TOKENS = {
    {"wain", "WAIN"}, {"int", "INT"}, {"if", "IF"}, {"else", "ELSE"},
    {"while", "WHILE"}, {"println", "PRINTLN"}, {"putchar", "PUTCHAR"},
    {"getchar", "GETCHAR"}, {"return", "RETURN"}, {"NULL", "NULL"},
    {"new", "NEW"}, {"delete", "DELETE"}
};

bool isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isKeyword(const std::string& s) {
    return KEYWORDS.find(s) != KEYWORDS.end();
}

bool isValidNum(const std::string& s) {
   if (s.length()>1 && s[0] == '0') {
    return false;
   }
   double num = std::stod(s);
   if (num > 2147483647.0) {
    return false;
   }
    return true;
}

int main() {
    std::istream& in = std::cin;
    std::unordered_map<std::string, bool> states;
    std::unordered_map<std::string, std::unordered_map<char, std::string>> transitions;
    std::string initialState = "START";
    bool comment = false;
  
    // States
    states[initialState] = false;
    states["ID"] = true;
    states["NUM"] = true;
    states["LPAREN"] = true;
    states["RPAREN"] = true;
    states["LBRACE"] = true;
    states["RBRACE"] = true;
    states["RETURN"] = true;
    states["IF"] = true;
    states["ELSE"] = true;
    states["WHILE"] = true;
    states["PRINTLN"] = true;
    states["PUTCHAR"] = true;
    states["GETCHAR"] = true;
    states["WAIN"] = true;
    states["BECOMES"] = true;
    states["INT"] = true;
    states["EQ"] = true;
    states["EXC"] = false;
    states["NE"] = true;
    states["LT"] = true;
    states["GT"] = true;
    states["LE"] = true;
    states["GE"] = true;
    states["PLUS"] = true;
    states["MINUS"] = true;
    states["STAR"] = true;
    states["SLASH"] = true;
    states["PCT"] = true;
    states["COMMA"] = true;
    states["SEMI"] = true;
    states["NEW"] = true;
    states["DELETE"] = true;
    states["LBRACK"] = true;
    states["RBRACK"] = true;
    states["AMP"] = true;
    states["NULL"] = true;
    states["COMMENT"] = false;

    // Transitions
    transitions[initialState]['('] = "LPAREN";
    transitions[initialState][')'] = "RPAREN";
    transitions[initialState]['{'] = "LBRACE";
    transitions[initialState]['}'] = "RBRACE";
    transitions[initialState]['='] = "BECOMES";
    transitions[initialState]['+'] = "PLUS";
    transitions[initialState]['-'] = "MINUS";
    transitions[initialState]['*'] = "STAR";
    transitions[initialState]['/'] = "SLASH";
    transitions[initialState]['%'] = "PCT";
    transitions[initialState][','] = "COMMA";
    transitions[initialState][';'] = "SEMI";
    transitions[initialState]['['] = "LBRACK";
    transitions[initialState][']'] = "RBRACK";
    transitions[initialState]['&'] = "AMP";
    transitions[initialState]['!'] = "EXC";
    transitions[initialState]['<'] = "LT";
    transitions[initialState]['>'] = "GT";
    transitions["EXC"]['='] = "NE";

    for (char c = '0'; c <= '9'; ++c) {
        transitions[initialState][c] = "NUM";
        transitions["NUM"][c] = "NUM";
        transitions["ID"][c] = "ID";
    }

    for (char c = 'a'; c <= 'z'; ++c) {
        transitions[initialState][c] = "ID";
        transitions["ID"][c] = "ID";
    }

    for (char c = 'A'; c <= 'Z'; ++c) {
        transitions[initialState][c] = "ID";
        transitions["ID"][c] = "ID";
    }

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string s;
        
        while (ss >> s) {
            if (s.find("//") == 0) break; // Skipping the line after you see a comment
            if (comment) {
                comment = false;
                break;
            }
            std::string token = "";
            std::string currentState = initialState;

            while (!s.empty()) {
                char a = s[0];

                if (transitions.find(currentState) != transitions.end() && transitions[currentState].find(a) != transitions[currentState].end()) {
                    token += a;
                    s = s.substr(1);
                    currentState = transitions[currentState][a];

                    if (currentState == "NUM" && token[0] == '0' && token.length() > 1) {
                        std::cerr << "ERROR" << std::endl;
                        return 0;
                    }
                } else {
                    if (states[currentState]) {
                        if (currentState == "BECOMES" && !s.empty() && s[0] == '=') {
                            token += s[0];
                            s = s.substr(1);
                            currentState = "EQ";
                        } else if (currentState == "LT" && !s.empty() && s[0] == '=') {
                            token += s[0];
                            s = s.substr(1);
                            currentState = "LE";
                        } else if (currentState == "GT" && !s.empty() && s[0] == '=') {
                            token += s[0];
                            s = s.substr(1);
                            currentState = "GE";
                        } else if (currentState == "SLASH" && !s.empty() && s[0] == '/') {
                            currentState = "COMMENT";
                            break;
                        } else {
                            if (currentState == "ID" && isKeyword(token)) {
                                currentState = KEYWORD_TOKENS.at(token);
                            }
                            std::cout << currentState << " " << token << std::endl;
                            currentState = initialState;
                            token = "";
                        }
                    } else {
                        std::cerr << "ERROR" << std::endl;
                        return 0;
                    }
                }
            }

            if (states[currentState]) {
                if (currentState == "ID" && isKeyword(token)) {
                    currentState = KEYWORD_TOKENS.at(token);
                } else if (currentState == "NUM" && !isValidNum(token)) {
                    std::cerr << "ERROR" << std::endl;
                    return 0;
                }
                std::cout << currentState << " " << token << std::endl;
            } else if (currentState == "COMMENT") {
              break;
            } else {
                std::cerr << "ERROR" << std::endl;
                return 0;
            }
        }
    }

    return 0;
}
