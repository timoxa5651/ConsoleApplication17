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

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::exception;



int main()
{
	Compiler compiler = Compiler();
	CompilationResult* result = compiler.Compile("../input.txt");
	if (result->GetString().find("Failed to read")) {
		delete result;
		result = compiler.Compile("input.txt");
	}
	cout << result->GetString() << endl;
	delete result;
}