#include "Parser.h"
#include "OCompiler.h"
#include "Lexeme.h"

ParserException::ParserException() {
	lexemeType = ELexemeType::Null;
	lexeme = "";
	lexemeNum = 1;
	context = "";
	line = 0;
}

ParserException::ParserException(ELexemeType lexemeType, const std::string& lexeme, int64_t lexemeNum, const std::string& context, int64_t line) {
	this->lexeme = lexeme;
	this->lexemeType = lexemeType;
	this->lexemeNum = lexemeNum;
	this->context = context;
	this->line = line;
}

ParserException::ParserException(const Lexeme& lexeme, int64_t lexemeNum, const std::string& context) {
	this->lexeme = lexeme.string;
	this->lexemeType = lexeme.type;
	this->lexemeNum = lexemeNum;
	this->context = context;
	this->line = lexeme.line;
}

std::string ParserException::what() {
	return "Error in lexeme " + lexeme + "   lexeme type: " + g_LexemeTypeToStr[lexemeType] + "  Context: " + context + "  Line: " + std::to_string(line);
}




Parser::Parser(const vector<LexemeSyntax>& lexemes) {
    std::vector<Lexeme> arr;
    for (auto& lex : lexemes) {
        if (lex.type == ELexemeType::Null) continue;
        arr.emplace_back(lex.type, LexemeTypeDataToStr(lex), lex.line);
    }
    arr.emplace_back(ELexemeType::Null, "End", INT_MAX);
	this->input_ = arr;
	this->currentLexemeIdx = 0;
	this->deepestException.lexemeNum = -2;
    this->MovePtr(0);

	this->currentScope = new FunctionScope(nullptr);
}
Parser::~Parser() {
	if (this->currentScope) {
		this->currentScope->RemoveParent();
		delete this->currentScope;
		this->currentScope = nullptr;
	}
}

void Parser::MovePtr(int idx) {
	this->currentLexemeIdx += idx;
	this->currentLexemeIdx = std::max(this->currentLexemeIdx, 0);
	this->currentLexemeIdx = std::min((size_t)this->currentLexemeIdx, this->input_.size() - 1);
    this->curLexeme_ = this->input_[this->currentLexemeIdx];
}

bool Parser::Check(CompilationResult* result) {
	try {
		//ReadLexeme();
		poliz = Program();
        poliz.PrintPoliz();
        poliz.PrintFuncRegistry();
	}
	catch (ParserException& exception) {
		if (exception.lexemeNum < deepestException.lexemeNum) {
			result->SetException(deepestException.what());
		}
		else {
			result->SetException(exception.what());
		}
		return false;
	}
	return true;
}

void Parser::ReadLexeme() {
	this->MovePtr(1);
}

void Parser::ClassStatement() {
    Function();
}

void Parser::ClassBlock() {
	FunctionScope* prevScope = this->currentScope;
	this->currentScope = new FunctionScope(this->currentScope);

    if (curLexeme_.string != "{") {
        throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no opening curly bracket in block definition");
    }
    ReadLexeme();
    while (curLexeme_.string != "}") {
        ClassStatement();
        ReadLexeme();
    }
    if (curLexeme_.string != "}") {
        throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no closing curly bracket in block definition");
    }
	delete this->currentScope;
	this->currentScope = prevScope;
}

Poliz Parser::Program() {
    Poliz programPoliz;
	try {
		bool wasMain = false;
		while (curLexeme_.string == "function" || curLexeme_.string == "class") {

            if (curLexeme_.string == "class") {
                ReadLexeme();
				this->isInAssign = true;
                Name();
				this->isInAssign = false;
				if (!this->declaredClasses.insert(curLexeme_.string).second) {
					throw ParserException(curLexeme_, this->currentLexemeIdx, "Class " + curLexeme_.string + " was already declared");
				}
				this->currentClass = curLexeme_.string;
                ReadLexeme();
                if (curLexeme_.string != "(") {
                    throw ParserException(curLexeme_, this->currentLexemeIdx,
                                          "expected ( in class declaration");
                }
                ReadLexeme();
                Num();
                ReadLexeme();
                if (curLexeme_.string != ")") {
                    throw ParserException(curLexeme_, this->currentLexemeIdx,
                                          "expected ) in class declaration");
                }
                ReadLexeme();
                ClassBlock();
                ReadLexeme();
				this->currentClass = "";
            } else {

                ReadLexeme();
				DeclaredFunction procFunction;
				procFunction.name = curLexeme_.string;
                if (this->FunctionExists(curLexeme_.string))
                    throw ParserException(curLexeme_, this->currentLexemeIdx,
                                          "function " + curLexeme_.string + " declared multiple times");
                //programPoliz.addFunction(curLexeme_.string);
                if (curLexeme_.string == "main") {
                    ReadLexeme();
                    if (curLexeme_.string != "(") {
                        throw ParserException(curLexeme_, this->currentLexemeIdx,
                                              "there is no opening bracket in function declaration");
                    }
                    ReadLexeme();
                    if (curLexeme_.string != ")") {
                        throw ParserException(curLexeme_, this->currentLexemeIdx,
                                              "there is no closing bracket in function declaration");
                    }
                    ReadLexeme();
                    programPoliz += Block();
                    ReadLexeme();
                    wasMain = true;
                    break;
                } else {
                    //ReadLexeme();
                    std::string funcName = curLexeme_.string;
                    auto func = Function();
                    programPoliz.addFunction(funcName, func);
//                    programPoliz += Function();
                    ReadLexeme();
                }
            }
		}
		if (curLexeme_.string != "End") {
			//                throw ParserException(curLexeme_, this->currentLexemeIdx, "incorrect function declaration");
			throw ParserException(curLexeme_, this->currentLexemeIdx,
				"incorrect function declaration");
		}
		if (!wasMain) {
			throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no main() in program");
		}
	}
	catch (ParserException& exception) {
		this->isInAssign = false;
		if (exception.lexemeNum < deepestException.lexemeNum && exception.lexemeNum >= 0) {
			throw deepestException;
		}
		else {
			throw exception;
		}
	}

    return programPoliz;
}

Poliz Parser::Function() {
	if (curLexeme_.type != ELexemeType::Variable) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "unnamed function");
	}
	DeclaredFunction func;
	func.name = curLexeme_.string;
	func.numArgs = -1;
	if (!this->currentClass.empty() && this->declaredFunctions[this->currentClass].find(func) != this->declaredFunctions[this->currentClass].end())
		throw ParserException(curLexeme_, this->currentLexemeIdx, "function " + curLexeme_.string + " declared multiple times");
	func.numArgs = 0;
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no opening bracket in function declaration");
	}
	ReadLexeme();
	std::set<string> args;
	if (curLexeme_.string != ")") {
		args = FunctionArgumentsDeclaration();
		func.numArgs = args.size();
		ReadLexeme();
	}
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no closing bracket in function declaration");
	}
	

	this->declaredFunctions[this->currentClass].insert(func);
	FunctionScope* prevScope = this->currentScope;
	this->currentScope = new FunctionScope(this->currentScope);
	for (const string& s : args)
		this->currentScope->InsertVariable(s);
	ReadLexeme();
	auto res = Block();

	delete this->currentScope;
	this->currentScope = prevScope;
    return res;
}

std::set<string> Parser::FunctionArgumentsDeclaration() {
	std::set<string> args;
	this->isInAssign = true;
	Name();
	args.insert(curLexeme_.string);
	this->isInAssign = false;
	ReadLexeme();
	while (curLexeme_.string == ",") {
		ReadLexeme();
		this->isInAssign = true;
		Name();
		if(!args.insert(curLexeme_.string).second)
			throw ParserException(curLexeme_, this->currentLexemeIdx, "Argument declared twice");
		this->isInAssign = false;
		ReadLexeme();
	}
	this->MovePtr(-1);
	return args;
}

Poliz Parser::Block() {
	if (curLexeme_.string != "{") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no opening curly bracket in block definition");
	}
	ReadLexeme();
	FunctionScope* prevScope = this->currentScope;
	this->currentScope = new FunctionScope(this->currentScope);
    Poliz block;
	while (curLexeme_.string != "}") {
		block += Statement();
		ReadLexeme();
	}
	delete this->currentScope;
	this->currentScope = prevScope;
	if (curLexeme_.string != "}") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "there is no closing curly bracket in block definition");
	}
    return block;
}

Poliz Parser::Statement() {
    return Exp();
}

Poliz Parser::ValueExp() {
    return MultivariateAnalyse({ &Parser::Value, &Parser::Container });
}
Poliz Parser::Operand() {
	return MultivariateAnalyse({ &Parser::ListElement, &Parser::FunctionCall, &Parser::Name, &Parser::String, &Parser::Num });
}

Poliz Parser::Value() {
    return Priority1();
}

Poliz Parser::Priority1() {
	auto res = Priority2();
	ReadLexeme();
	while (curLexeme_.string == "||") {
        auto cmd = curLexeme_.string;
        ReadLexeme();
        auto operand = Priority2();
        ReadLexeme();
        res += operand;
        res.addEntry(PolizCmd::Operation, cmd);
	}
	this->MovePtr(-1);
    return res;
}

Poliz Parser::Priority2() {
	auto res = Priority3();
	ReadLexeme();
	while (curLexeme_.string == "&&") {
        auto cmd = curLexeme_.string;
        ReadLexeme();
        auto operand = Priority3();
        ReadLexeme();
        res += operand;
        res.addEntry(PolizCmd::Operation, cmd);
	}
	this->MovePtr(-1);
    return res;
}

Poliz Parser::Priority3() {
	auto res =Priority4();
	ReadLexeme();
	while (curLexeme_.string == "==" || curLexeme_.string == "!=") {
        auto cmd = curLexeme_.string;
        ReadLexeme();
        auto operand = Priority4();
        ReadLexeme();
        res += operand;
        res.addEntry(PolizCmd::Operation, cmd);
	}
	this->MovePtr(-1);
    return res;
}

Poliz Parser::Priority4() {
	auto res =Priority5();
	ReadLexeme();
	while (curLexeme_.string == ">=" || curLexeme_.string == "<=" || curLexeme_.string == "<" || curLexeme_.string == ">") {
        auto cmd = curLexeme_.string;
        ReadLexeme();
        auto operand = Priority5();
        ReadLexeme();
        res += operand;
        res.addEntry(PolizCmd::Operation, cmd);
	}
	this->MovePtr(-1);
    return res;
}

Poliz Parser::Priority5() {
	auto res = Priority6();
	ReadLexeme();
	while (curLexeme_.string == "+" || curLexeme_.string == "-") {
        auto cmd = curLexeme_.string;
		ReadLexeme();
		auto operand = Priority6();
		ReadLexeme();
        res += operand;
        res.addEntry(PolizCmd::Operation, cmd);
	}
	this->MovePtr(-1);
    return res;
}

Poliz Parser::Priority6() {
	auto res = Priority7();
	ReadLexeme();
	while (curLexeme_.string == "*" || curLexeme_.string == "/" || curLexeme_.string == "//" || curLexeme_.string == "%") {
        auto cmd = curLexeme_.string;
		ReadLexeme();
		auto operand = Priority7();
		ReadLexeme();
        res += operand;
        res.addEntry(PolizCmd::Operation, cmd);
	}
	this->MovePtr(-1);
    return res;
}

Poliz Parser::Priority7() {
	if (curLexeme_.string == "-" || curLexeme_.string == "!") {
        auto cmd = curLexeme_.string;
		ReadLexeme();
		auto operand = Priority8();
        operand.addEntry(PolizCmd::UnOperation, cmd);
        return operand;
	}
	else {
        return Priority8();
	}
}

Poliz Parser::Priority8() {
	if (curLexeme_.string == "(") {
		ReadLexeme();
		auto exp = Priority1();
		ReadLexeme();
		if (curLexeme_.string != ")") {
			throw ParserException(curLexeme_, this->currentLexemeIdx, "missed closing bracket");
		}
        return exp;
	}
	else {
        return Operand();
	}
}


Poliz Parser::FunctionCall() {
	this->isInFuncCall = true;
    Poliz funcName;
	try {
		funcName = Name();
        funcName.changeEntryCmd(funcName.GetCurAddress(), PolizCmd::Call);
	}
	catch (const ParserException& ex) {
		this->isInFuncCall = false;
		throw;
	}
	this->isInFuncCall = false;
	DeclaredFunction currentFunc;
	currentFunc.name = this->lastReadName;
	currentFunc.numArgs = -1;
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx - 1, "missed opening bracket in function call");
	}
	ReadLexeme();
	int passedParams = 0;
    std::pair<int, Poliz> args;
    Poliz res;
	if (curLexeme_.string != ")") {
        args = Arguments();
		passedParams = args.first;
        args.second.Reverse();
        res += args.second;
        res += funcName;
	}
	else {
		return funcName;
	}
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "missed closing bracket in function call");
	}

	const DeclaredFunction& dFunc = *this->declaredFunctions[this->currentClass].find(currentFunc);
	if (dFunc.numArgs != passedParams) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid arguments " + std::to_string(passedParams) + " != " + std::to_string(dFunc.numArgs));
	}
    return res;
}

std::pair<int, Poliz> Parser::Arguments() {
	int numArgs = 1;
	auto param = ValueExp();
	ReadLexeme();
	while (curLexeme_.string == ",") {
		ReadLexeme();
		param += ValueExp();
		ReadLexeme();
		numArgs += 1;
	}
	this->MovePtr(-1);
	return {numArgs, param};
}

Poliz Parser::Container() {
    return MultivariateAnalyse({&Parser::FunctionCall, &Parser::List, &Parser::String });
}

Poliz Parser::String() {
	if (curLexeme_.type != ELexemeType::LiteralStr && curLexeme_.type != ELexemeType::LiteralChar) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid string literal");
	}
    Poliz res;
    res.addEntry(PolizCmd::Str, "\"" + curLexeme_.string + "\"");
    return res;
}

Poliz Parser::Name() {
	if (curLexeme_.type == ELexemeType::Keyword) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "variable can not be named by keyword");
	}
	if (curLexeme_.type != ELexemeType::Variable) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid variable name");
	}
	DeclaredFunction func;
	func.name = curLexeme_.string;
	func.numArgs = -1;
	bool isFunction = this->declaredFunctions[this->currentClass].find(func) != this->declaredFunctions[this->currentClass].end();
	if (!this->isInFuncCall) {
		if (isFunction) {
			throw ParserException(curLexeme_, this->currentLexemeIdx, "unable to reference function");
		}
	}

	if (!isFunction && !this->isInAssign && !this->currentScope->HasVariable(curLexeme_.string)) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "undefined variable");
	}
	this->lastReadName = curLexeme_.string;
    Poliz res;
    res.addEntry(PolizCmd::Var, curLexeme_.string);
    return res;
//    if (isInFuncCall) {
//        poliz.addEntryToBlock(PolizCmd::Call, curLexeme_.string);
//    } else {
//        poliz.addEntryToBlock(PolizCmd::Var, curLexeme_.string);
//    }
}

Poliz Parser::Num() {
	if (curLexeme_.type != ELexemeType::LiteralDouble && curLexeme_.type != ELexemeType::LiteralNum32 && curLexeme_.type != ELexemeType::LiteralNum64) {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid number");
	}
    Poliz res;
    res.addEntry(PolizCmd::Const, curLexeme_.string);
    return res;
//    poliz.addEntryToBlock(PolizCmd::Const, curLexeme_.string);
}

Poliz Parser::List() {
	return MultivariateAnalyse({ &Parser::TemporaryList, &Parser::Name });
}

Poliz Parser::ListElement() {
	auto arrayName = Name();
    arrayName.changeEntryCmd(arrayName.GetCurAddress(), PolizCmd::ArrayAccess);

//    poliz.changeEntryCmdInBlock(poliz.GetCurAddress(), PolizCmd::ArrayAccess);
	ReadLexeme();
	if (curLexeme_.string != "[") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening box bracket in getting list element");
	}
	ReadLexeme();
	auto index = MultivariateAnalyse({ &Parser::Name, &Parser::Num, &Parser::ListElement });
	ReadLexeme();
	if (curLexeme_.string != "]") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing box bracket in getting list element");
	}
    auto res = index + arrayName;
    return res;
}

Poliz Parser::Exp() {
	return MultivariateAnalyse({/*&Parser::VariableDeclaration, &Parser::ListDeclaration,*/ &Parser::FunctionCall,
						 &Parser::SpecialOperators, &Parser::ConditionalSpecialOperators, &Parser::Assign, &Parser::Return }, true);
}

Poliz Parser::Return() {
	if (curLexeme_.string != "return") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid return operator");
	}
	ReadLexeme();
    Poliz val;
	if (curLexeme_.string != ";") {
		val += ValueExp();
	}
	else {
		this->MovePtr(-1);
	}
    val.addEntry(PolizCmd::Ret, "");
    return val;
}

Poliz Parser::Assign() {
	auto assignTo = MultivariateAnalyse({ &Parser::ListElement, &Parser::Name }, false, true);
	string newVar = "";
	bool flag = false;
	if (curLexeme_.type == ELexemeType::Variable) {
		newVar = curLexeme_.string;
		flag = true;
	}
	ReadLexeme();
	if (curLexeme_.string != "=") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid assignment operator");
	}

//    poliz.addEntryBlockToPoliz();

	ReadLexeme();
	auto value = ValueExp();

//    poliz.addEntryToBlock(PolizCmd::Operation, "=");
	if (flag) {
		this->currentScope->InsertVariable(newVar);
	}
    Poliz res = assignTo + value;
    res.addEntry(PolizCmd::Operation, "=");
    return res;
}

void Parser::VariableDeclaration() {
	if (curLexeme_.string != "var") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected keyword var");
	}
	ReadLexeme();
	Name();
	ReadLexeme();
	if (curLexeme_.string == "=") {
		ReadLexeme();
		ValueExp();
	}
	else {
		this->MovePtr(-1);
	}

}

Poliz Parser::TemporaryList() {
    Poliz res;

	if (curLexeme_.string != "[") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening box bracket in list declaration");
	}
	ReadLexeme();
    std::pair<int, Poliz> args;
	if (curLexeme_.string != "]") {
		args = Arguments();
		ReadLexeme();
	}
    args.second.Reverse();
    res += args.second;
    res.addEntry(PolizCmd::Array, std::to_string(args.first));

	if (curLexeme_.string != "]") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing box bracket in list declaration");
	}
    return res;
}

Poliz Parser::SpecialOperators() {
	return MultivariateAnalyse({ &Parser::InputOperator, &Parser::OutputOperator });
}

Poliz Parser::InputOperator() {
	if (curLexeme_.string != "read") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid input operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in function call");
	}
	ReadLexeme();
	auto inArg = InputArguments();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in function call");
	}
    Poliz res;
    res.addEntry(PolizCmd::Call, "read");
    res += inArg;
    return res;
}

Poliz Parser::OutputOperator() {
	if (curLexeme_.string != "print") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid output operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in function call");
	}
	ReadLexeme();
	auto args = Arguments();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in function call");
	}
    Poliz res;
    res.addEntry(PolizCmd::Call, "print");
    res += args.second;
    return res;
}

Poliz Parser::InputArguments() {
	return MultivariateAnalyse({ &Parser::Name, &Parser::ListElement });
}

Poliz Parser::ConditionalSpecialOperators() {
    return MultivariateAnalyse({ &Parser::For, &Parser::While, &Parser::If });
}

Poliz Parser::If() {
	if (curLexeme_.string != "if") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid if operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in if structure");
	}
	ReadLexeme();
	auto val = ValueExp();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in if structure");
	}
	ReadLexeme();
	auto block = Block();
	ReadLexeme();
    val.addEntry(PolizCmd::Jz, std::to_string(block.GetSize() + 2));
    val += block;

    Poliz eBlock;
	if (curLexeme_.string == "else") {
        eBlock = Else();
	}
	else {
		this->MovePtr(-1);
	}
    val.addEntry(PolizCmd::Jump, std::to_string(eBlock.GetSize() + 1));
    val += eBlock;
    return val;
}

Poliz Parser::Else() {
	if (curLexeme_.string != "else") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid else operator");
	}
	ReadLexeme();
	return MultivariateAnalyse({ &Parser::Block, &Parser::If });
}

Poliz Parser::For() {
    Poliz res;
    auto tmpVarName = "__arr" + std::to_string(nextTmpVarSuffix) + "__";
    auto tmpItrName = "__itr" + std::to_string(nextTmpVarSuffix++) + "__";
	if (curLexeme_.string != "for") {
        --nextTmpVarSuffix;
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid for operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
        --nextTmpVarSuffix;
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in for structure");
	}

	ReadLexeme();
	this->isInAssign = true;
	auto itr = Name();
	this->isInAssign = false;
	ReadLexeme();
	if (curLexeme_.string != "in") {
        --nextTmpVarSuffix;
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected keyword IN");
	}
	ReadLexeme();
	auto container = Container();
	ReadLexeme();
	if (curLexeme_.string != ")") {
        --nextTmpVarSuffix;
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in for structure");
	}
	ReadLexeme();
	auto block = Block();


    res.addEntry(PolizCmd::Var, tmpVarName);
    res += container;
    res.addEntry(PolizCmd::Operation, "=");
//    res += itr;
    res.addEntry(PolizCmd::Var, tmpItrName);
    res.addEntry(PolizCmd::Const, "0");
    res.addEntry(PolizCmd::Operation, "=");

    int conditionFlag = res.GetSize();

    res.addEntry(PolizCmd::Var, tmpItrName);
    res.addEntry(PolizCmd::ArraySize, tmpVarName);
    res.addEntry(PolizCmd::Jge, std::to_string(block.GetSize() + itr.GetSize() + 10));

    res += itr;
    res.addEntry(PolizCmd::Var, tmpItrName);
    res.addEntry(PolizCmd::ArrayAccess, tmpVarName);
    res.addEntry(PolizCmd::Operation, "=");

    res += block;
    res.addEntry(PolizCmd::Var, tmpItrName);
    res.addEntry(PolizCmd::Var, tmpItrName);
    res.addEntry(PolizCmd::Const, "1");
    res.addEntry(PolizCmd::Operation, "+");
    res.addEntry(PolizCmd::Operation, "=");
    res.addEntry(PolizCmd::Jump, std::to_string(conditionFlag - res.GetSize()));
    --nextTmpVarSuffix;
    return res;
}

Poliz Parser::While() {
	if (curLexeme_.string != "while") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "invalid while operator");
	}
	ReadLexeme();
	if (curLexeme_.string != "(") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected opening bracket in while structure");
	}
	ReadLexeme();
	auto val = ValueExp();
	ReadLexeme();
	if (curLexeme_.string != ")") {
		throw ParserException(curLexeme_, this->currentLexemeIdx, "expected closing bracket in while structure");
	}
	ReadLexeme();
	auto block = Block();
    int jumpAddress = 0;
    val.addEntry(PolizCmd::Jz, std::to_string(block.GetSize() + 2));
    val += block;
    val.addEntry(PolizCmd::Jump, std::to_string(jumpAddress - val.GetSize()));
    return val;
}

Poliz Parser::MultivariateAnalyse(const std::vector<Poliz (Parser::*)()>& variants, bool isCheckEndLineSymbol, bool isAssign) {
	int64_t pos = (int64_t)this->currentLexemeIdx - 1;
	bool flag = true;
	ParserException exception_(ELexemeType::Null, "", -1, "", 0);
    Poliz correctBranch;
	for (auto fun : variants) {
		try {
			this->MovePtr(pos - this->currentLexemeIdx);
			ReadLexeme();
			if (fun == &Parser::Name) {
				this->isInAssign = isAssign;
			}
			correctBranch = (this->*fun)();
			if (isCheckEndLineSymbol && fun != &Parser::ConditionalSpecialOperators) {
				ReadLexeme();
				if (curLexeme_.string != ";") {
					throw ParserException(curLexeme_, this->currentLexemeIdx, "expected end of line symbol");
				}
			}
		}
		catch (ParserException& exception) {
			if (exception.lexemeNum > exception_.lexemeNum) {
				exception_ = exception;
				if (exception.lexemeNum > deepestException.lexemeNum) {
					deepestException = exception;
				}
			}
			continue;
		}
		flag = false;
		break;
	}
	this->isInAssign = false;
	if (flag) {
		throw exception_;
	}
    return correctBranch;
}


