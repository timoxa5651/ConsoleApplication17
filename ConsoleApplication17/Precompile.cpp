#include "Precompile.h"

RuntimeType* Precompile::Type_Null() {
	RuntimeType* type = new RuntimeType("Null", ERuntimeType::Null, 0);
	type->SetNativeCtor([](RuntimeVar* var, void* data) {

	});
	return type;
}

RuntimeType* Precompile::Type_Int64() {
	RuntimeType* type = new RuntimeType("Int64", ERuntimeType::Int64, 9);
	type->SetNativeCtor([](RuntimeVar* var, void* data) {
		*(int64_t*)var->dataBlob = *(int64_t*)data;
	});
	//type->SetMethod
	return type;
}

void Precompile::CreateTypes(RuntimeCtx* ctx) {
	ctx->AddType(Precompile::Type_Null());
	ctx->AddType(Precompile::Type_Int64());
}