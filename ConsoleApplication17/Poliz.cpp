//
// Created by Семён Чурбенко on 24.03.2023.
//

#include "Poliz.h"

void Poliz::addFunction(const std::string& name, Poliz& funcPoliz) {
    functionsRegistry.insert({name, funcPoliz});
}

void Poliz::pushCallStack(int address) {
    callStack.push_back(address);
}

int Poliz::getFunctionAddress(std::string name) {
    //return functionsRegistry[name];
    return 0;
}

int Poliz::GetReturnAddress() {
    int returnAddress = callStack.back();
    callStack.pop_back();
    return returnAddress;
}

void Poliz::addEntry(PolizCmd operation, std::string operand) {
    poliz.emplace_back( poliz.empty() ? 0 : poliz.back().id + 1, operation, operand);
}

void Poliz::addEntry(PolizEntry entry) {
    entry.id = poliz.empty() ? 0 : poliz.back().id + 1;
    poliz.emplace_back(entry);
}

void Poliz::changeEntryCmd(int address, PolizCmd cmd) {
    poliz[address].cmd = cmd;
}

void Poliz::PrintPoliz() const {
    std::cout << "Poliz\n";
    int cnt = 0;
    for (auto& elem : poliz) {
        std::cout << cnt++ << "\t" << PolizCmdToStr(elem.cmd) << " " << elem.operand << '\n';
    }
}

void Poliz::PrintFuncRegistry() const {
    std::cout << "________________________\n";
    for (const auto& func : functionsRegistry) {
        std::cout << "Name\t" << func.first << '\n';
        func.second.PrintPoliz();
        std::cout << "________________________\n";
    }
}

void Poliz::Reverse() {
    std::reverse(poliz.begin(), poliz.end());
}

