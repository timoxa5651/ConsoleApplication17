#include "Runtime.h"
#include "Precompile.h"
#include "Parser.h"
#include <cassert>

void RuntimeMethod::FromPoliz(RuntimeCtx* ctx, const std::vector<PolizEntry>& poliz) {
	std::vector<RuntimeInstr> cmd;

	std::stack<PolizEntry> stack;

	int retCnt = 1;
	int ctorCnt = 1;

	for (int i = 0; i < poliz.size(); ++i) {
		auto& entry = poliz[i];

		//auto Pop

		auto CreateScriptingInst = [&cmd, &ctorCnt](const RuntimeInstr& inInstr, const PolizEntry& entry) -> std::string {
			if (entry.cmd == PolizCmd::Var) { // already scripted
				return entry.operand;
			}

			RuntimeInstr instr(RuntimeInstrType::Ctor);
			std::string retName = "$ctor" + std::to_string(ctorCnt++);
			instr.AddParam(retName);

			if (entry.cmd == PolizCmd::Str) {
				instr.AddParam(Hash{}("String"));
				instr.AddParam(entry.operand);
			}
			else if (entry.cmd == PolizCmd::ConstInt) {
				instr.AddParam(Hash{}("Int64"));
				instr.AddParam(strtoll(entry.operand.c_str(), 0, 10));
			}
			else {
				assert(false);
			}
			cmd.push_back(instr);
			return retName;
		};

		switch (entry.cmd) {
		case PolizCmd::Operation: {
			PolizEntry pr2 = stack.top();
			stack.pop();
			PolizEntry pr1 = stack.top();
			stack.pop();

			ERuntimeCallType operType = ERuntimeCallType_FromString(entry.operand);
			RuntimeInstr oper(RuntimeInstrType::Operation);

			std::string retName;
			if (operType == ERuntimeCallType::Assign) {
				assert(pr1.cmd == PolizCmd::Var);
				retName = pr1.operand;
			}
			else {
				retName = "$ret" + std::to_string(retCnt++);
			}
			oper.AddParam(retName);
			oper.AddParam(operType);

			oper.AddParam(CreateScriptingInst(oper, pr1));
			oper.AddParam(CreateScriptingInst(oper, pr2));

			stack.push(PolizEntry{ -1, PolizCmd::Var, retName });
			cmd.push_back(oper);
			break;
		}
		case PolizCmd::Call: {
			RuntimeInstr call(RuntimeInstrType::Call);

			std::string retName = "$ret" + std::to_string(retCnt++);
			call.AddParam(retName);
			call.AddParam(entry.operand);

			RuntimeMethod* def = ctx->GetMethod(Hash{}(entry.operand));
			for (int i = 0; i < def->GetParamCount(); ++i) {
				PolizEntry pr = stack.top();
				stack.pop();
				call.AddParam(CreateScriptingInst(call, pr));
			}
			stack.push(PolizEntry{ -1, PolizCmd::Var, retName });

			cmd.push_back(call);
			break;
		}

		case PolizCmd::Jz: {

		}

						 // pass
		default: {
			stack.push(entry);
			break;
		}
		}

	}


	for (auto& instr : cmd) {
		std::cout << RuntimeInstrType_ToString(instr.opcode) << "  ";
		for (int i = 0; i < instr.GetParamCount(); ++i) {
			std::cout << instr.GetParamString(ctx, i) << " ";
		}
		std::cout << std::endl;
	}
}

std::string RuntimeInstr::GetParamString(RuntimeCtx* ctx, int idx) {
	auto& value = this->params[idx];
	if (value.type() == typeid(std::string))
		return std::any_cast<std::string>(value);
	if (value.type() == typeid(int64_t))
		return std::to_string(std::any_cast<int64_t>(value));
	if (value.type() == typeid(TID)) {
		if (ctx) {
			return ctx->GetType(std::any_cast<TID>(value))->GetName();
		}
		return std::to_string(std::any_cast<TID>(value));
	}
	if (value.type() == typeid(ERuntimeCallType))
		return ERuntimeCallType_ToString(std::any_cast<ERuntimeCallType>(value));
	assert(false);
}

RuntimeCtx::RuntimeCtx() {
	this->executor = new RuntimeExecutor();
	Precompile::CreateTypes(this);
}
RuntimeCtx::~RuntimeCtx() {
	if (this->executor) {
		delete this->executor;
		this->executor = nullptr;
	}
}

RuntimeMethod* RuntimeCtx::GetMethod(HashType methodName) {
	auto it = this->regMethods.find(methodName);
	if (it == this->regMethods.end())
		return nullptr;
	return it->second;
}
RuntimeType* RuntimeCtx::GetType(TID typeName) {
	auto it = this->regTypes.find(typeName);
	if (it == this->regTypes.end())
		return nullptr;
	return it->second;
}

void RuntimeCtx::AddPoliz(Parser* parser, Poliz* root) {
	// add types


	// add methods
	auto plist = parser->GetFunctions();

	for (auto& func : plist) {
		RuntimeMethod* method = new RuntimeMethod(func.name, func.numArgs);
		this->regMethods[Hash{}(func.name)] = method;
	}

	for (auto& [h, method] : this->regMethods) {
		const auto& pz = root->functionsRegistry[method->GetName()];
		method->FromPoliz(this, pz.poliz);
	}
}

void RuntimeCtx::AddType(RuntimeType* type) {
	this->regTypes[type->GetTID()] = type;
}
