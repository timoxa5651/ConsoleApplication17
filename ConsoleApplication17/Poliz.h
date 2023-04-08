//
// Created by Семён Чубенко on 24.03.2023.
//

#ifndef CONSOLEAPPLICATION17_POLIZ_H
#define CONSOLEAPPLICATION17_POLIZ_H

#include "vector"
#include "string"
#include "map"
#include "stack"
#include "iostream"
#include "algorithm"


enum class PolizCmd{
    Const,
    Var,
    Str,
    Call,
    Jump,
    Jz,
    Je,
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
        case PolizCmd::Const:           return "Const";
        case PolizCmd::Var:             return "Var";
        case PolizCmd::Str:             return "Str";
        case PolizCmd::Call:            return "Call";
        case PolizCmd::Jump:            return "Jump";
        case PolizCmd::Jz:              return "Jz";
        case PolizCmd::Je:              return "Je";
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

class PolizEntry{
public:
    PolizEntry(int id, PolizCmd cmd, const std::string& operand){
        this->id = id;
        this->cmd = cmd;
        this->operand = operand;
    }

    int id;
    PolizCmd cmd;
    std::string operand;
};


class Poliz {
    std::vector<PolizEntry> poliz;
    std::map<std::string, Poliz> functionsRegistry;
    std::vector<int> callStack;
public:

    void changeEntryCmd(int address, PolizCmd cmd);
    void addEntry(PolizEntry entry);
    void addEntry(PolizCmd operation, std::string operand);
    void addFunction(const std::string& name, Poliz& funcPoliz);
    void pushCallStack(int address);
    void Reverse();
    int getFunctionAddress(std::string name);
    int GetReturnAddress();
    int GetCurAddress() {return int(poliz.size()) - 1; };
    int GetSize() { return poliz.size(); };


    void PrintPoliz() const;
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



#endif //CONSOLEAPPLICATION17_POLIZ_H
