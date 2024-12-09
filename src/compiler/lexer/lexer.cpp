#include <iostream>
#include <vector>
#include <regex>

#include "include/token.h"
#include "include/lexer.h"

using namespace std;

struct TokenPositionBusy {
    int start;
    int end;
};

string getTokenTypeString(int type) {
    switch (type) {
        case ID:
            return "ID";
        case NUMBER:
            return "NUMBER";
        case STRING:
            return "STRING";
        case SEMICOLON:
            return "SEMICOLON";
        case WHITESPACE:
            return "WHITESPACE";
        case LBRACKET:
            return "LBRACKET";
        case RBRACKET:
            return "RBRACKET"; 
        case LSQUARE_BRACKET:
            return "LSQUARE_BRACKET";
        case RSQUARE_BRACKET:
            return "RSQUARE_BRACKET";
        case LOBJECT_BRACKET:
            return "LOBJECT_BRACKET";
        case ROBJECT_BRACKET:
            return "ROBJECT_BRACKET";
        case COMMA:
            return "COMMA";
        case DOT:
            return "DOT";
        case NEWLINE:
            return "NEWLINE";
        case ASSIGN:
            return "ASSIGN";
        case PLUS:
            return "PLUS";
        case MINUS:
            return "MINUS";
        case DIV:
            return "DIV";
        case MUL:
            return "MUL";
        case DEF:
            return "DEF";
        case EQ:
            return "EQ";
        case NOTEQ:
            return "NOTEQ";
        case BIGGER:
            return "BIGGER";
        case SMALLER:
            return "SMALLER";
        case BIGGER_OR_EQ:
            return "BIGGER_OR_EQ";
        case SMALLER_OR_EQ:
            return "SMALLER_OR_EQ";
        case AND:
            return "AND";
        case OR:
            return "OR";
        case BEGIN:
            return "BEGIN";
        case TRUE:
            return "TRUE";
        case FALSE:
            return "FALSE";
        case NULLT:
            return "NULL";
        case IF:
            return "IF";
        case ELSE:
            return "ELSE";
        case END:
            return "END";
        case RETURN:
            return "RETURN";
        case DELAY:
            return "DELAY";
        case OUTPUT:
            return "OUTPUT";
        case USING:
            return "USING";
        default:
            return "";
    }
}

bool compareTokens(Token* first, Token* second) {
    return first->getPosition() < second->getPosition();
}

Lexer::Lexer(string code) {
    _code = code;

    _tokenTypesPatterns = {
        make_pair("\".+?\"", STRING),
        make_pair("\'.+?\'", STRING),

        make_pair("true", TRUE),
        make_pair("false", FALSE),
        make_pair("null", NULLT),

        make_pair(";", SEMICOLON),
        make_pair("\\s+", WHITESPACE),

        make_pair("\\(", LBRACKET),
        make_pair("\\)", RBRACKET),

        make_pair("\\[", LSQUARE_BRACKET),
        make_pair("\\]", RSQUARE_BRACKET),

        make_pair("\\{", LOBJECT_BRACKET),
        make_pair("\\}", ROBJECT_BRACKET),

        make_pair(",", COMMA),
        make_pair("\\.", DOT),

        make_pair("\\+", PLUS),
        make_pair("\\-", MINUS),
        make_pair("\\/", DIV),
        make_pair("\\*", MUL),

        make_pair("!=", NOTEQ),
        make_pair("==", EQ),

        make_pair(">=", BIGGER_OR_EQ),
        make_pair("<=", SMALLER_OR_EQ),

        make_pair(":=", ASSIGN),
 
        make_pair(">", BIGGER),
        make_pair("<", SMALLER),

        make_pair("&", AND),
        make_pair("\\?", OR),

        make_pair(":", BEGIN),
        make_pair("end", END),
        make_pair("fn", DEF),
        
        make_pair("if", IF),
        make_pair("else", ELSE),

        make_pair("return", RETURN),
        make_pair("delay", DELAY),
        make_pair("output", OUTPUT),

        make_pair("using", USING),

        make_pair("[a-zA-Z_][a-zA-Z0-9_]*", ID),
        make_pair("[+-]?([0-9]*[.])?[0-9]+", NUMBER),
    };
}

vector<Token*> Lexer::tokenize(bool logs) {
    vector<TokenPositionBusy> busy;

    for (pair<string, TokenType> v: _tokenTypesPatterns) {
        string regexString = v.first;
        TokenType tokenType = v.second;

        regex rx(regexString);

        auto begin = sregex_iterator {_code.begin(), _code.end(), rx};
        auto end = sregex_iterator();

        for (sregex_iterator i = begin; i != end; ++i) {
            sort(_tokens.begin(), _tokens.end(), compareTokens);

            TokenType tokenTypeDynamic = tokenType;

            string str = i->str();

            for (pair<string, TokenType> pattern: _tokenTypesPatterns) {
                if (pattern.first == str && pattern != v) { tokenTypeDynamic = pattern.second; }
            }

            int strLen = str.length();
            int pos = i->position();
            int endPos = pos + strLen;

            bool block = false;

            for (auto v: busy) {
                if (pos >= v.start && endPos <= v.end) block = true;
            }

            if (block) continue;

            TokenPositionBusy p;
            p.start = pos;
            p.end = endPos;

            busy.push_back(p);

            Token* token = new Token(str, tokenTypeDynamic, pos, endPos);

            _tokens.push_back(token);
        }
    }

    vector<Token*> tokens;
    copy_if(_tokens.begin(), _tokens.end(), std::back_inserter(tokens), [](Token* token) {
        return token->getType() != WHITESPACE && token->getType() != NEWLINE;
    });

    sort(tokens.begin(), tokens.end(), compareTokens);

    for (Token* token: tokens) {
        if (token->getType() == STRING) {
            token->value = token->value.substr(1, token->value.length() - 2);
        }
    }

    if (logs) {
        for (Token* v: tokens) {
            cout << " [ " + getTokenTypeString(v->getType()) + " ] [ " + v->value + " ] [ " + to_string(v->getPosition()) + " ] [ " + to_string(v->getEndPosition()) + " ] " << endl;
        }
    }

    return tokens;
}