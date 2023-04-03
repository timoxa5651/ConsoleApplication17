//
// Created by Семён Чубенко on 24.03.2023.
//

#include "Poliz.h"

void Poliz::addFunction(std::string name, int address) {
    functionsRegistry.insert({name, address});
}

void Poliz::pushCallStack(int address) {
    callStack.push(address);
}

int Poliz::getFunctionAddress(std::string name) {
    return functionsRegistry[name];
}

int Poliz::GetReturnAddress() {
    int returnAddress = callStack.top();
    callStack.pop();
    return returnAddress;
}

void Poliz::addEntry(PolizCmd operation, std::string& operand) {
    poliz.emplace_back(poliz.back().id + 1, operation, operand);
}

void Poliz::addEntryBlock(std::vector<std::pair<PolizCmd, std::string>> &block) {
    for (auto elem : block) {
        addEntry(elem.first, elem.second);
    }
}
