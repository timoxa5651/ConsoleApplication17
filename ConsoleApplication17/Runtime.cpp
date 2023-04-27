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
			assert(def); // "No registered method found"
			int paramCount = def->GetParamCount();
			if (paramCount < 0) {
				PolizEntry paramCountDyn = stack.top();
				stack.pop();
				assert(paramCountDyn.cmd == PolizCmd::ConstInt);
				paramCount = std::stoi(paramCountDyn.operand);
			}
			for (int i = 0; i < paramCount; ++i) {
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
			bool hasValue = std::stoll(entry.operand) != 0;
			RuntimeInstr ret(RuntimeInstrType::Ret);
			if (hasValue) {
				PolizEntry actionVar = stack.top();
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

	//print
	for (auto& instr : cmd) {
		std::cout << RuntimeInstrType_ToString(instr.opcode) << "  ";
		for (int i = 0; i < instr.GetParamCount(); ++i) {
			std::cout << instr.GetParamString(ctx, i) << " ";
		}
		std::cout << std::endl;
	}

	// insert fn
	RuntimeInstr* alloc = ctx->AllocateFunction(this, cmd.size());
	std::copy(cmd.data(), cmd.data() + cmd.size(), alloc);
}

std::vector<uint8_t> RuntimeInstr::GetRawParam(size_t idx) const {
	auto& value = this->params[idx];

	ByteStream bf_write;

	if (value.type() == typeid(std::string))
		bf_write.Write(std::any_cast<std::string>(value));
	if (value.type() == typeid(int64_t))
		bf_write.Write(std::any_cast<int64_t>(value));
	if (value.type() == typeid(HashType)) 
		bf_write.Write(std::any_cast<HashType>(value));
	if (value.type() == typeid(ERuntimeCallType))
		bf_write.Write(std::any_cast<ERuntimeCallType>(value));
	
	

	return bf_write.GetBuffer();
}
std::string RuntimeInstr::GetParamString(RuntimeCtx* ctx, size_t idx) const {
	auto& value = this->params[idx];
	if (value.type() == typeid(std::string))
		return std::any_cast<std::string>(value);
	if (value.type() == typeid(int64_t))
		return std::to_string(std::any_cast<int64_t>(value));
	if (value.type() == typeid(HashType)) {
		if (ctx) {
			return ctx->GetType(std::any_cast<HashType>(value))->GetName();
		}
		return std::to_string(std::any_cast<HashType>(value));
	}
	if (value.type() == typeid(ERuntimeCallType))
		return ERuntimeCallType_ToString(std::any_cast<ERuntimeCallType>(value));
	std::cout << "Unknown ret type " << std::string(value.type().name()) << std::endl;
	assert(false);
}

RuntimeVar* RuntimeExecutor::CreateVar(){
	for(auto& [begin, pool] : this->varPool){
		RuntimeVar* var = pool.Pop();
		if(var) return var;
	}
	// create pool
	auto pool = VarPool<RuntimeVar>(1000);
	this->varPool[pool.block] = pool;
	return pool.Pop();
}
const LocalVarState& RuntimeExecutor::GetLocal(std::string name) {
	auto it = this->locals.find(name);
	if(it == this->locals.end()){
		LocalVarState state;
		state.var = this->CreateVar();
		
		this->locals[name] = state;
		return state;
	}
	return it->second;
}

RuntimeVar* RuntimeExecutor::ExecuteInstr(RuntimeCtx* ctx, RuntimeInstr* instr) {
	printf("Executing instruction %d\n", instr->opcode);

	if(instr->opcode == RuntimeInstrType::Ctor){
		auto& bret = instr->GetParam<std::string>(0);
		const auto& local = this->GetLocal(bret);
		if(local.var->heldType->GetTypeEnum() != ERuntimeType::Null)
			local.var->NativeTypeConvert(ctx->GetType(ERuntimeType::Null)); // reset var so we don't convert
		TID targetType = instr->GetParam<TID>(1);
		local.var->NativeTypeConvert(ctx->GetType(targetType));

		ByteStream writer;
		for(size_t i = 2; i < 2 + instr->GetParamCount(); ++i){
			writer.Write(instr->GetRawParam(i));
		}
		local.var->NativeCtor(writer.GetBuffer());
	}
	return nullptr;
}

RuntimeVar* RuntimeExecutor::CallMethod(RuntimeCtx* ctx, RuntimeMethod* method, const RuntimeParamPack& params) {
	if (method->IsNative()) {
		return method->NativeCall(ctx, params);
	}
	this->ip = method->GetVA();

	// scripted
	while (!this->isErrored) {
		RuntimeInstr* instr = ctx->GetInstr(this->ip);
		RuntimeVar* var = this->ExecuteInstr(ctx, instr);
		if (instr->opcode == RuntimeInstrType::Ret) {
			// handle Ret
			return var;
		}
	}
	return nullptr;
}


RuntimeCtx::RuntimeCtx() {
	this->executor = new RuntimeExecutor();
	Precompile::CreateTypes(this);
	
	// set default
	this->defaultTypes[(int)ERuntimeType::Int64] = this->GetType(Hash{}("Int64"));
	this->defaultTypes[(int)ERuntimeType::String] = this->GetType(Hash{}("String"));
	this->defaultTypes[(int)ERuntimeType::Null] = this->GetType(Hash{}("Null"));
	this->defaultTypes[(int)ERuntimeType::Array] = this->GetType(Hash{}("Array"));
	this->defaultTypes[(int)ERuntimeType::Double] = this->GetType(Hash{}("Double"));

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
RuntimeType* RuntimeCtx::GetType(ERuntimeType typeEnum){
	return this->defaultTypes[static_cast<int>(typeEnum)];
}

void RuntimeCtx::AddPoliz(Parser* parser, Poliz* root) {
	// add types


	// add methods
	auto plist = parser->GetFunctions();

	for (auto& func : plist) {
		if (this->GetMethod(func.name)) {
			continue; // don't override
		}
		RuntimeMethod* method = new RuntimeMethod(func.name, func.numArgs);
		this->regMethods[Hash{}(func.name)] = method;
	}

	for (auto& [h, method] : this->regMethods) {
		if (method->IsNative()) continue;
		const auto& pz = root->functionsRegistry[method->GetName()];
		std::cout << "--------            Generating method " << method->GetName() << std::endl;
		method->FromPoliz(this, pz.poliz);
		std::cout << std::endl;
	}
}

int64_t RuntimeCtx::ExecuteRoot(std::string functionName) {
	RuntimeMethod* method = this->GetMethod(functionName);
	if (!method) {
		return 1;
	}
	this->executor->Reset();
	RuntimeVar* ret = this->executor->CallMethod(this, method, RuntimeParamPack());
	if (!ret) {
		return 1;
	}
	if (ret->heldType->GetTypeEnum() != ERuntimeType::Int64) {
		// cast...how
	}
	return ret->data.i64;
}

void RuntimeCtx::AddType(RuntimeType* type) {
	this->regTypes[type->GetTID()] = type;
}
void RuntimeCtx::AddMethod(RuntimeMethod* method) {
	this->regMethods[Hash{}(method->GetName())] = method;
}

RuntimeInstr* RuntimeCtx::AllocateFunction(RuntimeMethod* method, size_t size) {
	method->SetVA(this->instrHolder.size());
	this->instrHolder.resize(this->instrHolder.size() + size);
	return &this->instrHolder[this->instrHolder.size() - size];
}
RuntimeInstr* RuntimeCtx::GetInstr(REG idx) {
	return &this->instrHolder[idx];
}