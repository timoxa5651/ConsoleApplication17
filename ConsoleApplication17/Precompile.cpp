#include "Precompile.h"

RuntimeType* Precompile::Type_Null() {
	RuntimeType* type = new RuntimeType("Null", ERuntimeType::Null, 0);
	type->SetNativeCtor([](RuntimeVar* var, void* data) {

	});
	return type;
}

RuntimeType* Precompile::Type_Int64() {
	RuntimeType* type = new RuntimeType("Int64", ERuntimeType::Int64, 8);
	type->SetNativeCtor([](RuntimeVar* var, void* data) {
		*(int64_t*)var->dataBlob = *(int64_t*)data;
	});
	return type;
}

void Precompile::AddReservedMethods(RuntimeCtx* ctx) {
	ctx->AddMethod(new RuntimeMethod("print", -1, [](RuntimeCtx* ctx, const std::vector<RuntimeVar*>& params) -> RuntimeVar* {
		printf("Script: print called...\n");
		return nullptr;
	}));

	ctx->AddMethod(new RuntimeMethod("read", -1, [](RuntimeCtx* ctx, const std::vector<RuntimeVar*>& params) -> RuntimeVar* {
		return nullptr;
	}));
}

void Precompile::CreateTypes(RuntimeCtx* ctx) {
	ctx->AddType(Precompile::Type_Null());
	ctx->AddType(Precompile::Type_Int64());

	AddReservedMethods(ctx);
}