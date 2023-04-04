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
    PolizEntry(int id, PolizCmd operation, std::string operand){
        this->id = id;
        this->operation = operation;
        this->operand = operand;
    }

    int id;
    PolizCmd operation;
    std::string operand;
};


class Poliz {
    std::vector<PolizEntry> poliz;
    std::map<std::string, int> functionsRegistry;
    std::stack<int> callStack;
public:


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
            this->poliz.emplace_back(elem);
        }
        return *this;
    }

    Poliz operator +(Poliz other) {
        Poliz res;
        for (auto& elem : other.poliz) {
            res.poliz.emplace_back(elem);
        }
        return res;
    }

};



#endif //CONSOLEAPPLICATION17_POLIZ_H
