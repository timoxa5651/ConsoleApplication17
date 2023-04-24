#include "Runtime.h"
#include "Precompile.h"
#include "Parser.h"
#include <cassert>
#include <queue>

void RuntimeMethod::FromPoliz(RuntimeCtx* ctx, const std::vector<PolizEntry>& poliz) {
	std::vector<RuntimeInstr> cmd;

	std::stack<PolizEntry> stack;

	int retCnt = 1;
	int ctorCnt = 1;

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
			instr.AddParam(std::stoll(entry.operand.c_str()));
		}
		else if (entry.cmd == PolizCmd::Null) {
			instr.AddParam(Hash{}("Null"));
		}
		else {
			assert(false);
		}
		cmd.push_back(instr);
		return retName;
	};	

	std::vector<int64_t> addrMap(poliz.size());
	for (int64_t i = 0; i < poliz.size(); ++i) {
		addrMap[i] = cmd.size();
		auto& entry = poliz[i];

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
		case PolizCmd::Array: {
			size_t arraySize = std::stoll(entry.operand.c_str());
			RuntimeInstr array(RuntimeInstrType::Array);

			std::string retName = "$ret" + std::to_string(retCnt++);
			array.AddParam(retName);

			for (size_t i = 0; i < arraySize; ++i) {
				PolizEntry pr = stack.top();
				stack.pop();
				array.AddParam(CreateScriptingInst(array, pr));
			}

			stack.push(PolizEntry{ -1, PolizCmd::Var, retName });
			cmd.push_back(array);
			break;
		}
		case PolizCmd::Jz: {
			PolizEntry actionVar = stack.top();
			int64_t delta = std::stoll(entry.operand);
			RuntimeInstr jz(RuntimeInstrType::Jz);
			jz.AddParam<int64_t>(delta + i);
			jz.AddParam(CreateScriptingInst(jz, actionVar));

			cmd.push_back(jz);
			break;
		}
		case PolizCmd::Jump: {
			PolizEntry actionVar = stack.top();
			int64_t delta = std::stoll(entry.operand);
			RuntimeInstr jmp(RuntimeInstrType::Jmp);
			jmp.AddParam<int64_t>(delta + i);

			cmd.push_back(jmp);
			break;
		}

		case PolizCmd::Ret: {
			PolizEntry actionVar = stack.top();
			bool hasValue = std::stoll(entry.operand) != 0;
			RuntimeInstr ret(RuntimeInstrType::Ret);
			if (hasValue) {
				ret.AddParam(CreateScriptingInst(ret, actionVar));
			}
			else {
				ret.AddParam(CreateScriptingInst(ret, PolizEntry(-1, PolizCmd::Null, "")));
			}
			cmd.push_back(ret);
			break;
		}
		
		default: {
			stack.push(entry);
			break;
		}
		}
	}

	// jmp rebase
	for (int64_t i = 0; i < cmd.size(); ++i) {
		auto& instr = cmd[i];
		if (instr.opcode != RuntimeInstrType::Jmp && instr.opcode != RuntimeInstrType::Jz)
			continue;
		int64_t& base = instr.RefParam<int64_t>(0);
		base = addrMap[base] - i - 1;
	}

	for (auto& instr : cmd) {
		std::cout << RuntimeInstrType_ToString(instr.opcode) << "  ";
		for (int i = 0; i < instr.GetParamCount(); ++i) {
			std::cout << instr.GetParamString(ctx, i) << " ";
		}
		std::cout << std::endl;
	}
}

std::string RuntimeInstr::GetParamString(RuntimeCtx* ctx, size_t idx) const {
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
