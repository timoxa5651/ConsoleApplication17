#pragma once

#include "vector"
#include "string"
#include "map"
#include "stack"
#include "iostream"
#include "algorithm"


enum class PolizCmd {
    Invalid,

    ConstInt,
    ConstDbl,
    Var,
    Str,
    Call,
    Jump,
    Jz,
    Jge,
    Array,
    Operation,
    ArrayAccess,
    ArraySize,
    UnOperation,
    Ret,
    Null
};

inline std::string PolizCmdToStr(PolizCmd cmd){
    switch (cmd) {
        case PolizCmd::ConstInt:        return "ConstInt";
        case PolizCmd::ConstDbl:        return "ConstDbl";
        case PolizCmd::Var:             return "Var";
        case PolizCmd::Str:             return "Str";
        case PolizCmd::Call:            return "Call";
        case PolizCmd::Jump:            return "Jump";
        case PolizCmd::Jz:              return "Jz";
        case PolizCmd::Jge:             return "Jge";
        case PolizCmd::Array:           return "Array";
        case PolizCmd::ArrayAccess:     return "ArrayAccess";
        case PolizCmd::ArraySize:       return "ArraySize";
        case PolizCmd::Operation:       return "Operation";
        case PolizCmd::UnOperation:     return "UnOperation";
        case PolizCmd::Ret:             return "Ret";
        case PolizCmd::Null:            return "Null";
        default:                        return "[Unknown cmd]";
    }
}

class PolizEntry {
public:
    PolizEntry(int id, PolizCmd cmd, const std::string& operand, int polizEntryIdx){
        this->id = id;
        this->cmd = cmd;
        this->operand = operand;
        this->polizEntryIdx = polizEntryIdx;
    }

    int id;
    PolizCmd cmd;
    std::string operand;
    int polizEntryIdx;
};

class Poliz {
    friend class RuntimeCtx;

    std::vector<PolizEntry> poliz;
    std::map<std::string, Poliz> functionsRegistry;
    std::vector<int> callStack;

    void PrintPoliz() const;
public:

    void changeEntryCmd(int address, PolizCmd cmd);
    const PolizEntry& getLastEntry() { return poliz.back(); };
    PolizCmd getLastEntryType() { return poliz.empty() ? PolizCmd::Invalid : poliz[poliz.size() - 1].cmd; };
    void addEntry(PolizEntry entry);
    void addEntry(PolizCmd operation, std::string operand, int lexemeIdx);
    void addFunction(const std::string& name, Poliz& funcPoliz);
    void pushCallStack(int address);
    void Reverse();
    int getFunctionAddress(std::string name);
    int GetReturnAddress();
    int GetCurAddress() {return int(poliz.size()) - 1; };
    int GetSize() { return poliz.size(); };

    void PrintFuncRegistry() const;


    Poliz& operator = (const Poliz& other){
        if (this != &other){
            poliz = other.poliz;
            functionsRegistry = other.functionsRegistry;
            callStack = other.callStack;
        }
        return *this;
    }

    Poliz& operator += (const Poliz& other) {
        for (auto& elem : other.poliz) {
            this->addEntry(elem);
        }
        for (auto& elem : other.callStack) {
            this->callStack.push_back(elem);
        }
        return *this;
    }

    Poliz operator +(const Poliz& other) {
        Poliz res;
        for (auto& elem : this->poliz) {
            res.addEntry(elem);
        }
        for (auto& elem : other.poliz) {
            res.addEntry(elem);
        }
        for (auto& elem : this->callStack) {
            res.callStack.push_back(elem);
        }
        for (auto& elem : other.callStack) {
            res.callStack.push_back(elem);
        }
        return res;
    }


};

