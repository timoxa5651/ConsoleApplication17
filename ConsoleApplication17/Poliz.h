//
// Created by Семён Чубенко on 24.03.2023.
//

#ifndef CONSOLEAPPLICATION17_POLIZ_H
#define CONSOLEAPPLICATION17_POLIZ_H

#include "vector"
#include "string"
#include "map"
#include "stack"


enum class PolizCmd{
    Const,
    Var,
    Str,
    Call,
    Jump,
    Jl,
    Je,
    Array,
    Operation,
    ArrayAccess
};

class PolizEntry{
public:
    PolizEntry(int id, PolizCmd cmd, std::string operand){
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
    std::map<std::string, int> functionsRegistry;
    std::vector<int> callStack;
public:

    void changeEntryCmd(int address, PolizCmd cmd);
    void addEntry(PolizEntry entry);
    void addEntry(PolizCmd operation, std::string operand);
    void addFunction(std::string name);
    void pushCallStack(int address);
    int getFunctionAddress(std::string name);
    int GetReturnAddress();
    int GetCurAddress() {return poliz.size() - 1; };


    Poliz& operator = (const Poliz& other){
        if (this != &other){
            poliz = other.poliz;
            functionsRegistry = other.functionsRegistry;
            callStack = other.callStack;
        }
        return *this;
    }

    Poliz& operator += (Poliz other) {
        for (auto& elem : other.poliz) {
            this->addEntry(elem);
        }
        for (auto& elem : other.callStack) {
            this->callStack.push_back(elem);
        }
        return *this;
    }

    Poliz operator +(Poliz other) {
        Poliz res;
        for (auto& elem : this->poliz) {
            res.addEntry(elem);
        }
        for (auto& elem : other.poliz) {
            res.addEntry(elem);
        }
        for (auto& elem : other.callStack) {
            res.callStack.push_back(elem);
        }
        return res;
    }

};



#endif //CONSOLEAPPLICATION17_POLIZ_H
