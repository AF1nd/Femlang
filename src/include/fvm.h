#ifndef FVM_H
#define FVM_H

#include <iostream>
#include <stack>
#include <vector>
#include <any>
#include <memory>
#include <optional>
#include <map>

using namespace std;

enum Bytecode {
    F_PUSH,
    F_SETVAR,
    F_GETVAR,

    F_LOADFUNC,
    F_LOADIFST,
    F_CALL,
    F_RETURN,
    F_DELAY,

    F_OUTPUT,

    F_ADD,
    F_MUL,
    F_DIV,
    F_SUB,

    F_EQ,
    F_NOTEQ,
    F_BIGGER,
    F_SMALLER,

    F_BIGGER_OR_EQ,
    F_SMALLER_OR_EQ,

    F_IF,
    F_ELSE,

    F_AND,
    F_OR,

    
};

struct InstructionOperrand {
    any operrand;

    virtual string tostring() const = 0; 

    virtual bool isEq(shared_ptr<InstructionOperrand> toEq) { return false; };
};

struct InstructionNullOperrand : InstructionOperrand {
    bool operrand = true;

    InstructionNullOperrand() = default;

    string tostring() const override {
        return "NULL";
    }

    bool isEq(shared_ptr<InstructionOperrand> toEq) override {
        if (auto casted = dynamic_pointer_cast<InstructionNullOperrand>(toEq)) return true;
        return false;
    }
};

struct InstructionNumberOperrand : InstructionOperrand {
    double operrand;

    InstructionNumberOperrand(double operrand) { this->operrand = operrand; };

    string tostring() const override {
        return to_string(operrand);
    }
    
    bool isEq(shared_ptr<InstructionOperrand> toEq) override {
        if (auto casted = dynamic_pointer_cast<InstructionNumberOperrand>(toEq)) return operrand == casted->operrand;
        return false;
    }
};

struct InstructionStringOperrand : InstructionOperrand {
    string operrand;

    InstructionStringOperrand(string operrand) { this->operrand = operrand; };

    string tostring() const override  {
        return operrand;
    }

    bool isEq(shared_ptr<InstructionOperrand> toEq) override {
        if (auto casted = dynamic_pointer_cast<InstructionStringOperrand>(toEq)) return operrand == casted->operrand;
        return false;
    }
};

struct InstructionBoolOperrand : InstructionOperrand {
    bool operrand;

    InstructionBoolOperrand(bool operrand) { this->operrand = operrand; };

    string tostring() const override  {
        string str = operrand == true ? "true" : "false";
        return str;
    }

    bool isEq(shared_ptr<InstructionOperrand> toEq) override {
        if (auto casted = dynamic_pointer_cast<InstructionBoolOperrand>(toEq)) return operrand == casted->operrand;
        return false;
    }
};

struct Instruction {
    optional<shared_ptr<InstructionOperrand>> operrand;
    Bytecode code;

    Instruction(Bytecode code, 
        optional<shared_ptr<InstructionOperrand>> operrand
    ) { this->code = code; this->operrand = operrand; };

    Instruction(Bytecode code) { this->code = code; };

    Instruction() = default;
};

struct FuncDeclaration {
    vector<Instruction> bytecode;
    vector<string> argsIds;
    string id;

    FuncDeclaration(vector<Instruction> bytecode, vector<string> argsIds, string id) { this->bytecode = bytecode; this->argsIds = argsIds, this->id = id; };
    FuncDeclaration() = default;
};

struct IfStatement {
    vector<Instruction> bytecode;
    vector<Instruction> elseBytecode;

    IfStatement(vector<Instruction> bytecode) { this->bytecode = bytecode; };
    IfStatement(vector<Instruction> bytecode, vector<Instruction> elseBytecode)  { this->bytecode = bytecode; this->elseBytecode = elseBytecode; };
    IfStatement() = default;
};

struct InstructionFunctionLoadOperrand : InstructionOperrand {
    FuncDeclaration operrand;

    InstructionFunctionLoadOperrand(FuncDeclaration operrand) { this->operrand = operrand; };

    string tostring() const override {
        return operrand.id;
    }
};

struct InstructionIfStatementLoadOperrand : InstructionOperrand {
    IfStatement operrand;

    InstructionIfStatementLoadOperrand(IfStatement operrand) { this->operrand = operrand; };

    string tostring() const override {
        return "IF_STMNT";
    }
};

class FVM {
    public:
        stack<shared_ptr<InstructionOperrand>> vmStack;
  
        shared_ptr<InstructionOperrand> run(vector<Instruction> bytecode, map<string, FuncDeclaration> functions = {}, map<string, shared_ptr<InstructionOperrand>> scope = {});
        FVM(bool logs);

        void push(shared_ptr<InstructionOperrand> operrand);

        shared_ptr<InstructionOperrand> pop();

        string getBytecodeString(vector<Instruction> bytecode);

        bool logs;
};

#endif