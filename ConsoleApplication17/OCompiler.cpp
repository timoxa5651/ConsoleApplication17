#include "OCompiler.h"
#include "Runtime.h"

CompilationResult::CompilationResult() {
	this->wasOk = true;
}
string CompilationResult::GetString() {
	string result;
	if (this->wasOk) {
		result = "Compile OK";
	}
	else {
		result = "Fail: " + string(this->compileException.what());
	}
	return result;
}

void CompilationResult::SetException(const CompileException& ex) {
	this->wasOk = false;
	this->compileException = ex;
}


bool FunctionScope::HasVariable(const string& variableName) {
	if (this->parent && this->parent->HasVariable(variableName)) {
		return true;
	}
	return this->declaredVaribles.find(variableName) != this->declaredVaribles.end();
}

void FunctionScope::InsertVariable(const string& variableName) {
	this->declaredVaribles.insert(variableName);
}

vector<LexemeSyntax> Compiler::GetLexems(string inputFile) {
	vector<string> resvOperators = { "!", "+", "*", "/", "-", "%", "=", "&&", "==", "!=", "||", "<", "<=", ">=", ">", "//" };
	vector<string> resvKeywords = { "while", "for", "if", "switch", "return", "function" };
	std::ifstream t(inputFile);
	if (!t.good()) {
		throw CompileException("Failed to read input file");
	}

	std::stringstream buffer;
	buffer << t.rdbuf();
	std::string input = buffer.str();

	vector<LexemeSyntax> result;
	result.reserve(100);

	int currentLine = 1;
	int lineStartPos = 0;

	Stream stream = input;
	while (!stream.is_end()) {
		char rd = stream.peek_char();
		if (rd == ' ' || rd == '\n' || rd == '\t') {
			if (rd == '\n') {
				currentLine += 1;
				lineStartPos = stream.get_cur();
			}
			stream.seek(1);
			result.push_back(LexemeSyntax(ELexemeType::Null, currentLine, stream.get_cur() - lineStartPos));
			continue;
		}
		else if (rd == '{' || rd == '}') {
			stream.seek(1);
			result.push_back(LexemeSyntax(ELexemeType::CurlyBrack, rd, currentLine, stream.get_cur() - lineStartPos));
			continue;
		}
		else if (rd == '[' || rd == ']') {
			stream.seek(1);
			result.push_back(LexemeSyntax(ELexemeType::BoxBrack, rd, currentLine, stream.get_cur() - lineStartPos));
			continue;
		}
		else if (rd == '(' || rd == ')') {
			stream.seek(1);
			result.push_back(LexemeSyntax(ELexemeType::RoundBrack, rd, currentLine, stream.get_cur() - lineStartPos));
			continue;
		}
		else if (rd == ',' || rd == ';') {
			stream.seek(1);
			result.push_back(LexemeSyntax(ELexemeType::Punctuation, rd, currentLine, stream.get_cur() - lineStartPos));
			continue;
		}
		else if (rd == '"' || rd == '\'') {
			stream.seek(1);
			string var_name;
			for (int i = 0; !stream.is_end(); ++i) {
				if (stream.peek_char() == rd) break;
				var_name += stream.read_char();
			}
			stream.seek(1);
			result.push_back(LexemeSyntax(rd == '"' ? ELexemeType::LiteralStr : ELexemeType::LiteralChar, var_name, currentLine, stream.get_cur() - lineStartPos));
			continue;
		}
		else if (rd >= '0' && rd <= '9') {
			int cr = stream.get_cur();
			int64_t num = stream.get_num(stream.get_cur(), true, true);
			if (!stream.is_end() && stream.peek_char() == '.') {
				stream.set_cur(cr);
				double num2 = stream.get_num_dbl(stream.get_cur(), true, true);
				result.push_back(LexemeSyntax(ELexemeType::LiteralDouble, num2, currentLine, stream.get_cur() - lineStartPos));
			}
			else {
				if (abs(num) >= INT_MAX) {
					result.push_back(LexemeSyntax(ELexemeType::LiteralNum64, num, currentLine, stream.get_cur() - lineStartPos));
				}
				else {
					result.push_back(LexemeSyntax(ELexemeType::LiteralNum32, num, currentLine, stream.get_cur() - lineStartPos));
				}
			}
			continue;
		}
		else {
			auto checkIfInList = [&](vector<string> list, bool isOperator) -> std::tuple<bool, string> {
				int cr = stream.get_cur();
				std::tuple<bool, string> result = { false, "" };
				for (int i = 0; !stream.is_end() && list.size(); ++i) {
					char chr = stream.read_char();
					for (auto it = list.begin(); it != list.end(); ) {
						if (it->at(i) != chr) {
							it = list.erase(it);
						}
						else if (it->size() == i + 1) {
							bool flag = isOperator;
							if (stream.is_end()) flag = true;
							else {
								char c = tolower(stream.peek_char());
								if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')) flag = true;
							}
							if (flag) {
								result = { true, *it };
							}
							it = list.erase(it);
						}
						else {
							++it;
						}
					}
				}
				stream.set_cur(cr);
				return result;
			};

			auto [validKeyword, str] = checkIfInList(resvKeywords, false);
			if (validKeyword) {
				stream.seek(str.size());
				result.push_back(LexemeSyntax(ELexemeType::Keyword, str, currentLine, stream.get_cur() - lineStartPos));
				continue;
			}
			auto [validOperator, str2] = checkIfInList(resvOperators, true);
			if (validOperator) {
				stream.seek(str2.size());
				result.push_back(LexemeSyntax(ELexemeType::Operator, str2, currentLine, stream.get_cur() - lineStartPos));
				continue;
			}

			string var_name;
			for (int i = 0; !stream.is_end(); ++i) {
				char c = tolower(stream.peek_char());
				if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9')) break;
				var_name += stream.read_char();
			}

			if (!var_name.size()) {
				result.push_back(LexemeSyntax(ELexemeType::Invalid, currentLine, stream.get_cur() - lineStartPos));
				stream.seek(1);
			}
			else {
				result.push_back(LexemeSyntax(ELexemeType::Variable, var_name, currentLine, stream.get_cur() - lineStartPos));
			}
		}
	}

	return result;
}

CompilationResult* Compiler::Compile(string inputFile) {
	CompilationResult* result = new CompilationResult();

	vector<LexemeSyntax> input;
	try {
		input = this->GetLexems(inputFile);
	}
	catch (const CompileException& ex) {
		result->SetException(ex);
		return result;
	}
	catch (const ReadException& ex) {
		result->SetException(ex);
		return result;
	}
	this->parser = new Parser(input);
	if (!this->parser->Check(result)) {
		return result;
	}
	this->parser->poliz.addFunction("main", this->parser->poliz);
	this->parser->poliz.PrintFuncRegistry();

	std::cout << result->GetString() << std::endl;

	this->runtime = new RuntimeCtx();
	this->runtime->AddPoliz(this->parser, &this->parser->poliz);

	int64_t ret = this->runtime->ExecuteRoot("main");
	printf("Main returned %lld\n", ret);
	if (ret != 0) {
		printf("[Script] Execution failed: %s\n", this->runtime->GetErrorString().c_str());
	}


	return result;
}

