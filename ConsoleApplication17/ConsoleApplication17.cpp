#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <exception>
#include <fstream>
#include <sstream>
#include <climits>
#include <set>
#include "Stream.h"
#include "Lexeme.h"
#include "OCompiler.h"

#include "Poliz.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::exception;

//Poliz f(){
//    Poliz c;
//    c.addEntry(PolizCmd::Var , "b");
//    return c;
//}

int main()
{
	Compiler compiler = Compiler();
	CompilationResult* result = compiler.Compile("../input.txt");
//	if (result->GetString().find("Failed to read")) {
//		delete result;
//		result = compiler.Compile("input.txt");
//	}
	cout << result->GetString() << endl;
	delete result;

//    Poliz c, d;
//    c.addEntry(PolizCmd::Var , "c");
//    d.addEntry(PolizCmd::Var , "d");
//    Poliz e = c + d;
//    std::cout << 1;
}