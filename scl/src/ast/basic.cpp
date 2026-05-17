#include "scl/ast/basic.hpp"
#include "scl/ast/type.hpp"

namespace scl {

TypeDecl::TypeDecl(misc::Token name, ict::UPtr<Type> &&t)
    :Decl(name, TypeType::create()), m_aliased(this, std::move(t)) {}

};
