#include <iostream>
#include <stack>
#include <vector>
#include <algorithm>
#include <string>
#include <chrono>
#include <thread>

#include "include/fvm.h"

using namespace std;

string opcodeToString(Bytecode opcode) {
    switch (opcode) {
        case F_PUSH:
            return "PUSH";
            break;
        case F_LOADK:
            return "LOADK";
            break;
        case F_GETK:
            return "GETK";
            break;
        case F_LOADFUNC:
            return "LOADFUNC";
            break;
        case F_CALL:
            return "CALL";
            break;
        case F_RETURN:
            return "RETURN";
            break;
        case F_DELAY:
            return "DELAY";
            break;
        case F_OUTPUT:
            return "OUTPUT";
            break;
         case F_ADD:
            return "ADD";
            break;
        case F_SUB:
            return "SUB";
            break;
        case F_MUL:
            return "MUL";
            break;
        case F_DIV:
            return "DIV";
            break;
        case F_EQ:
            return "EQ";
            break;
        case F_NOTEQ:
            return "NOTEQ";
            break;
        case F_AND:
            return "AND";
            break;
        case F_BIGGER:
            return "BIGGER";
            break;
        case F_SMALLER:
            return "SMALLER";
            break;
        case F_BIGGER_OR_EQ:
            return "BIGGER_OR_EQ";
            break;
        case F_SMALLER_OR_EQ:
            return "SMALLER_OR_EQ";
            break;
        default:
            break;
    }

    return "unknown";
};

bool binaryNumbersCondition(shared_ptr<InstructionOperrand> one, shared_ptr<InstructionOperrand> two, Bytecode opcode) {
    auto oneCasted = dynamic_pointer_cast<InstructionNumberOperrand>(one);
    auto twoCasted = dynamic_pointer_cast<InstructionNumberOperrand>(two);

    if (oneCasted && twoCasted) {
        if (opcode == F_BIGGER) return oneCasted->operrand > twoCasted->operrand;
        if (opcode == F_SMALLER) return oneCasted->operrand < twoCasted->operrand;
        if (opcode == F_BIGGER_OR_EQ) return oneCasted->operrand >= twoCasted->operrand;
        if (opcode == F_SMALLER_OR_EQ) return oneCasted->operrand <= twoCasted->operrand;
    } else throw runtime_error("FVM: BINARY CONDITION WITH OPCODE " + opcodeToString(opcode) + " CAN WORK ONLY WITH NUMBERS!");

    return false;
}

FVM::FVM(bool logs) {
    this->logs = logs;
}

shared_ptr<InstructionOperrand> FVM::run(vector<Instruction> bytecode, shared_ptr<Scope> scope, shared_ptr<Scope> parent) {
    if (logs) cout << getBytecodeString(bytecode) << endl;

    if (scope != nullptr && parent != nullptr) {
        for (pair<string, ScopeMember> member: parent->members) {
            if (scope->members.find(member.first) == scope->members.end()) scope->members.insert(member);
        }
    }

    for (Instruction code: bytecode) {
        switch (code.code) {
            case F_PUSH:
                {
                    bool hasVal = code.operrand.has_value();
                    shared_ptr<InstructionOperrand> val = code.operrand.value();
                    if (!hasVal) throw runtime_error("FVM: NO OPERRAND FOR PUSH");

                    push(code.operrand.value());
                }
                break;
            case F_IF:
                {
                    auto operrand = dynamic_pointer_cast<InstructionIfStatementLoadOperrand>(code.operrand.value());
                    if (!operrand) throw runtime_error("FVM: NO OPERRAND FOR IF INSTRUCTION");

                    IfStatement statement = operrand->operrand;

                    shared_ptr<InstructionOperrand> val = pop();

                    if (auto boolean = dynamic_pointer_cast<InstructionBoolOperrand>(val)) {
                        vector<Instruction> bytecode = statement.bytecode;
                        if (!boolean->operrand) bytecode = statement.elseBytecode;

                        if (!bytecode.empty()) {
                            shared_ptr<Scope> newScope = make_shared<Scope>();

                            shared_ptr<InstructionOperrand> result = run(bytecode, newScope, scope);

                            for (pair<string, ScopeMember> member: newScope->members) {
                                ScopeMember realMember = member.second;
                                
                                if (scope->members.find(member.first) != scope->members.end()) {
                                    scope->members[member.first] = realMember;
                                }
                            }

                            if (result != nullptr) return result;
                        }
                    }
                }
                break;
            case F_DELAY:
                {
                    auto val = dynamic_pointer_cast<InstructionNumberOperrand>(pop());
                    if (!val) throw runtime_error("FVM: DELAY ERROR, NO NUMBER IN STACK");

                    this_thread::sleep_for(chrono::duration<double>(val->operrand));
                }
                break;
            case F_RETURN:
                {
                    shared_ptr<InstructionNullOperrand> val = make_shared<InstructionNullOperrand>();
                    if (vmStack.empty()) return val;
                    
                    return pop();
                }
                break;
            case F_LOADFUNC:
                {
                    if (!code.operrand.has_value()) throw runtime_error("FVM: CANNOT LOAD FUNC! NO OPERRAND");

                    auto declr = dynamic_pointer_cast<InstructionFunctionLoadOperrand>(code.operrand.value());
                    FuncDeclaration fnDeclr = declr->operrand;
                    string id = fnDeclr.id;

                    fnDeclr.scope = scope;

                    scope->members[id] = fnDeclr;
                }
                break;
            case F_CALL:
                {
                    if (!code.operrand.has_value()) throw runtime_error("FVM: CANNOT CALL FUNC! NO OPERRAND");
                    auto funcId = dynamic_pointer_cast<InstructionStringOperrand>(code.operrand.value());

                    vector<shared_ptr<InstructionOperrand>> args;

                    string funcName = funcId->operrand;

                    FuncDeclaration funcDeclar;

                    try {
                        funcDeclar = get<FuncDeclaration>(scope->members.at(funcName));
                    }
                    catch(const exception& e) {
                        throw runtime_error("FVM: FUNCTION " + funcName + " DOESN'T EXIST");
                    }
                    

                    int argsNum = funcDeclar.argsIds.size();

                    for (size_t i = 0; i < argsNum; ++i) {
                        shared_ptr<InstructionOperrand> arg = pop();
                        args.push_back(arg);
                    }

                    shared_ptr<Scope> newScope = make_shared<Scope>();

                    for (size_t i = 0; i < argsNum; ++i) {
                        shared_ptr<InstructionOperrand> arg = args.at(i);
                        string id = funcDeclar.argsIds.at(i);
                        
                        newScope->members.insert({ id, arg });
                    }

                    shared_ptr<InstructionOperrand> result = run(funcDeclar.bytecode, newScope, funcDeclar.scope);

                    push(result != nullptr ? result : make_shared<InstructionNullOperrand>());
                }
                break;
            case F_LOADK:
                {
                    auto adr = dynamic_pointer_cast<InstructionStringOperrand>(code.operrand.value());
                    auto val = pop();

                    if (adr) {
                        scope->members[adr->operrand] = val;
                    }
                    else throw runtime_error("FVM: FOR SETVAR EXPECTED ADDRESS (OPERRAND 1)");
                }
                break;
            case F_GETK:
                {
                    auto adr = dynamic_pointer_cast<InstructionStringOperrand>(code.operrand.value());
                    if (adr) {
                        try {
                            ScopeMember val = scope->members.at(adr->operrand);
                            push(get<shared_ptr<InstructionOperrand>>(val));
                        }
                        catch (const exception& e) {
                            throw runtime_error("FVM: BY ADDRESS " + adr->operrand + " NOT FINDED ANYTHING");
                        }
                        
                    } else throw runtime_error("FVM: FOR GETVAR EXPECTED ADDERS (OPERRAND)");
                }
                break;
            case F_OUTPUT:
                {
                    shared_ptr<InstructionOperrand> val = pop();
                    cout << "OUTPUT: " + val->tostring() << endl;
                }
                break;
            case F_EQ:
                {
                    shared_ptr<InstructionOperrand> one = pop();
                    shared_ptr<InstructionOperrand> two = pop();

                    push(make_shared<InstructionBoolOperrand>(one->isEq(two)));
                }
                break;
            case F_NOTEQ:
                {
                    shared_ptr<InstructionOperrand> one = pop();
                    shared_ptr<InstructionOperrand> two = pop();

                    push(make_shared<InstructionBoolOperrand>(!(one->isEq(two))));
                }
                break;
            case F_BIGGER:
                {
                    shared_ptr<InstructionOperrand> one = pop();
                    shared_ptr<InstructionOperrand> two = pop();

                    push(make_shared<InstructionBoolOperrand>(binaryNumbersCondition(two, one, F_BIGGER)));
                }
                break;
            case F_SMALLER:
                {
                    shared_ptr<InstructionOperrand> one = pop();
                    shared_ptr<InstructionOperrand> two = pop();

                    push(make_shared<InstructionBoolOperrand>(binaryNumbersCondition(two, one, F_SMALLER)));
                }
                break;
            case F_BIGGER_OR_EQ:
                {
                    shared_ptr<InstructionOperrand> one = pop();
                    shared_ptr<InstructionOperrand> two = pop();

                    push(make_shared<InstructionBoolOperrand>(binaryNumbersCondition(two, one, F_BIGGER_OR_EQ)));
                }
                break;
            case F_SMALLER_OR_EQ:
                {
                    shared_ptr<InstructionOperrand> one = pop();
                    shared_ptr<InstructionOperrand> two = pop();

                    push(make_shared<InstructionBoolOperrand>(binaryNumbersCondition(two, one, F_SMALLER_OR_EQ)));
                }
                break;
            case F_AND:
                {
                    shared_ptr<InstructionOperrand> one = pop();
                    shared_ptr<InstructionOperrand> two = pop();

                    if (auto oneCasted = dynamic_pointer_cast<InstructionBoolOperrand>(one)) {
                        if (auto twoCasted = dynamic_pointer_cast<InstructionBoolOperrand>(two)) {
                            push(make_shared<InstructionBoolOperrand>(oneCasted->operrand == true && twoCasted->operrand == true));
                        }
                    }
                }
                break;
            case F_OR:
                {
                    shared_ptr<InstructionOperrand> one = pop();
                    shared_ptr<InstructionOperrand> two = pop();

                    bool isFalse = false;
                    if (auto casted = dynamic_pointer_cast<InstructionBoolOperrand>(two)) isFalse = !casted->operrand;

                    if (dynamic_pointer_cast<InstructionNullOperrand>(two) || isFalse) push(one);
                    else push(two);
                }
                break;
            case F_ADD:
                {
                    auto val1 = dynamic_pointer_cast<InstructionNumberOperrand>(pop());
                    auto val2 = dynamic_pointer_cast<InstructionNumberOperrand>(pop());

                    if (val1 && val2) push(make_shared<InstructionNumberOperrand>(val1->operrand + val2->operrand));
                    else throw runtime_error("FVM: ADD ERROR! OPERRANDS MUST BE A NUMBERS");
                }
                break;
            case F_SUB:
                {
                    auto val1 = dynamic_pointer_cast<InstructionNumberOperrand>(pop());
                    auto val2 = dynamic_pointer_cast<InstructionNumberOperrand>(pop());

                    if (val1 && val2) push(make_shared<InstructionNumberOperrand>(val2->operrand - val1->operrand));
                    else throw runtime_error("FVM: SUB ERROR! OPERRANDS MUST BE A NUMBERS");
                }
                break;
            case F_MUL:
                {
                    auto val1 = dynamic_pointer_cast<InstructionNumberOperrand>(pop());
                    auto val2 = dynamic_pointer_cast<InstructionNumberOperrand>(pop());

                    if (val1 && val2) push(make_shared<InstructionNumberOperrand>(val1->operrand * val2->operrand));
                    else throw runtime_error("FVM: MUL ERROR! OPERRANDS MUST BE A NUMBERS");
                }
                break;
            case F_DIV:
                {
                    auto val1 = dynamic_pointer_cast<InstructionNumberOperrand>(pop());
                    auto val2 = dynamic_pointer_cast<InstructionNumberOperrand>(pop());

                    if (val1 && val2) push(make_shared<InstructionNumberOperrand>(val2->operrand / val1->operrand));
                    else throw runtime_error("FVM: DIV ERROR! OPERRANDS MUST BE A NUMBERS");
                }
                break;
            default:
                break;
        }
    }

    return 0;
}

void FVM::push(shared_ptr<InstructionOperrand> operrand) {
    vmStack.push(operrand);
}

shared_ptr<InstructionOperrand> FVM::pop() {
    if (vmStack.empty()) throw runtime_error("FVM: CANNOT POP FROM EMPTY STACK");

    auto top = vmStack.top();
    vmStack.pop();
    return top;
}

string FVM::getBytecodeString(vector<Instruction> bytecode) {
    string str = "";

    for (Instruction code: bytecode) {
        string opStr;
        string opStr2;

        if (code.operrand.has_value()) {
            opStr = code.operrand.value()->tostring();
        }

        string opcodeName = opcodeToString(code.code);

        str += "\n  > " + to_string(code.code) + " | " + (opcodeName != "unknown" ? opcodeName : to_string(code.code)) + " " + opStr + " " + opStr2;
    }

    return "[ BYTECODE ]" + str;
}