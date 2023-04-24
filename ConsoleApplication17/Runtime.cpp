#include "Runtime.h"
#include "Precompile.h"

#pragma region RuntimeMethod
RuntimeMethod* RuntimeMethod::FromPoliz(RuntimeCtx* ctx, const std::vector<PolizEntry>& poliz) {
	RuntimeMethod* method = new RuntimeMethod();

	std::stack<PolizEntry> stack;
	for(int i = 0; i < poliz.size(); ++i){
		auto& entry = poliz[i];
		bool isPush = entry.cmd == PolizCmd::Const;

		if(isPush){
			stack.push(entry);
		}
		else{
			
		}
	}

	return method;
}
#pragma endregion RuntimeMethod

#pragma region RuntimeCtx
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


void RuntimeCtx::AddPoliz(Poliz* root) {
	// add types


	// add methods
	for (auto& [name, func] : root->functionsRegistry) {
		
	}
}

void RuntimeCtx::AddType(RuntimeType* type) {
	this->regTypes[type->GetTID()] = type;
}
#pragma endregion RuntimeCtx