//
// Created by Семён Чубенко on 24.03.2023.
//

#include "Poliz.h"

void Poliz::addFunction(std::string name) {
    functionsRegistry.insert({name, poliz.size()});
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
    poliz.emplace_back( poliz.empty() ? 0 : poliz.back().id + 1, operation, operand);
}

void Poliz::addEntryBlockToPoliz() {
    for (auto elem : entryBlock) {
        addEntry(elem.first, elem.second);
    }
    entryBlock.clear();
}

void Poliz::addEntryToBlock(PolizCmd operation, std::string operand) {
    entryBlock.emplace_back(operation, operand);
}

void Poliz::clearEntryBlock() {
    entryBlock.clear();
}
