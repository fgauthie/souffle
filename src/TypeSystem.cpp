/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TypeSystem.cpp
 *
 * Covers basic operations constituting Souffle's type system.
 *
 ***********************************************************************/

#include "TypeSystem.h"
#include "RamTypes.h"
#include "Util.h"
#include <cassert>

namespace souffle {

void SubsetType::print(std::ostream& out) const {
    out << getName() << " <: " << baseType;
}

void UnionType::add(const Type& type) {
    assert(environment.isType(type));
    elementTypes.push_back(&type);
}

void UnionType::print(std::ostream& out) const {
    out << getName() << " = "
        << join(elementTypes, " | ", [](std::ostream& out, const Type* type) { out << type->getName(); });
}

void RecordType::add(const std::string& name, const Type& type) {
    assert(environment.isType(type));
    fields.push_back(Field({name, type}));
}

void RecordType::print(std::ostream& out) const {
    out << getName() << " = ";
    if (fields.empty()) {
        out << "()";
        return;
    }
    out << "( " << join(fields, " , ", [](std::ostream& out, const RecordType::Field& f) {
        out << f.name << " : " << f.type.getName();
    }) << " )";
}

TypeSet TypeEnvironment::initializeConstantTypes() {
    auto& signedConstant = createType<ConstantType>("numberConstant");
    auto& floatConstant = createType<ConstantType>("floatConstant");
    auto& symbolConstant = createType<ConstantType>("symbolConstant");
    auto& unsignedConstant = createType<ConstantType>("unsignedConstant");

    return TypeSet(signedConstant, floatConstant, symbolConstant, unsignedConstant);
}

TypeSet TypeEnvironment::initializePrimitiveTypes() {
#define CREATE_PRIMITIVE(TYPE) \
    auto& TYPE##Type =         \
            createType<PrimitiveType>(#TYPE, static_cast<const ConstantType&>(getType(#TYPE "Constant")));

    CREATE_PRIMITIVE(number);
    CREATE_PRIMITIVE(float);
    CREATE_PRIMITIVE(symbol);
    CREATE_PRIMITIVE(unsigned);
    // auto& signedType = createType<PrimitiveType>("number", getType("numberConstant"));
    // auto& floatType = createType<PrimitiveType>("float", getType("floatConstant"));
    // auto& symbolType = createType<PrimitiveType>("symbol", getType("symbolConstant"));
    // auto& unsignedType = createType<PrimitiveType>("unsigned", getType("unsignedConstant"));

    return TypeSet(numberType, floatType, symbolType, unsignedType);

#undef CREATE_PRIMITIVE
}

bool TypeEnvironment::isType(const AstQualifiedName& ident) const {
    return types.find(ident) != types.end();
}

bool TypeEnvironment::isType(const Type& type) const {
    const Type& t = getType(type.getName());
    return t == type;
}

const Type& TypeEnvironment::getType(const AstQualifiedName& ident) const {
    auto it = types.find(ident);
    assert(it != types.end());
    return *(it->second);
}

TypeSet TypeEnvironment::getAllTypes() const {
    TypeSet res;
    for (const auto& cur : types) {
        res.insert(*cur.second);
    }
    return res;
}

void TypeEnvironment::addType(Type* type) {
    const AstQualifiedName& name = type->getName();
    assert(types.find(name) == types.end() && "Error: registering present type!");
    types[name] = std::unique_ptr<Type>(type);
}

namespace {

/**
 * A visitor for Types.
 */
template <typename R>
struct TypeVisitor {
    virtual ~TypeVisitor() = default;

    R operator()(const Type& type) const {
        return visit(type);
    }

    virtual R visit(const Type& type) const {
        //        std::cerr << type << std::endl;
        // check all kinds of types and dispatch
        if (auto* t = dynamic_cast<const ConstantType*>(&type)) {
            return visitConstantType(*t);
        }
        if (auto* t = dynamic_cast<const SubsetType*>(&type)) {
            return visitSubsetType(*t);
        }
        if (auto* t = dynamic_cast<const UnionType*>(&type)) {
            return visitUnionType(*t);
        }
        if (auto* t = dynamic_cast<const RecordType*>(&type)) {
            return visitRecordType(*t);
        }

        assert(false && "Unsupported type encountered!");
        return R();
    }

    virtual R visitConstantType(const ConstantType& type) const {
        return visitType(type);
    }

    virtual R visitSubsetType(const SubsetType& type) const {
        return visitType(type);
    }

    virtual R visitUnionType(const UnionType& type) const {
        return visitType(type);
    }

    virtual R visitRecordType(const RecordType& type) const {
        return visitType(type);
    }

    virtual R visitType(const Type& /*type*/) const {
        return R();
    }
};

/**
 * A visitor for types visiting each type only once (effectively breaking
 * recursive cycles).
 */
template <typename R>
class VisitOnceTypeVisitor : public TypeVisitor<R> {
protected:
    mutable std::map<const Type*, R> seen;

public:
    R visit(const Type& type) const override {
        auto pos = seen.find(&type);
        if (pos != seen.end()) {
            return pos->second;
        }
        auto& res = seen[&type];  // mark as seen
        return res = TypeVisitor<R>::visit(type);
    }
};

template <typename T>
bool isA(const Type& type) {
    return dynamic_cast<const T*>(&type);
}

template <typename T>
const T& as(const Type& type) {
    return static_cast<const T&>(type);
}

/**
 * Determines whether the given type is a sub-type of the given root type.
 */
bool isOfRootType(const Type& type, const Type& root) {
    struct visitor : public VisitOnceTypeVisitor<bool> {
        const Type& root;

        explicit visitor(const Type& root) : root(root) {}

        bool visitConstantType(const ConstantType& type) const override {
            return type == root;
        }
        bool visitSubsetType(const SubsetType& type) const override {
            return type == root || isOfRootType(type.getBaseType(), root);
        }
        bool visitUnionType(const UnionType& type) const override {
            return !type.getElementTypes().empty() &&
                   all_of(type.getElementTypes(), [&](const Type* cur) { return this->visit(*cur); });
        }

        bool visitType(const Type& /*unused*/) const override {
            return false;
        }
    };

    return visitor(root).visit(type);
}

bool isSubType(const Type& a, const UnionType& b) {
    // A is a subtype of b if it is in the transitive closure of b
    struct visitor : public VisitOnceTypeVisitor<bool> {
        const Type& target;
        explicit visitor(const Type& target) : target(target) {}

        bool visitConstantType(const ConstantType& type) const override {
            return target == type;
        }

        bool visitSubsetType(const SubsetType& type) const override {
            //            std::cerr << "type: " << type << " target: " << target << std::endl;
            if (target == type) {
                return true;
            }
            return this->visit(type.getBaseType());
        }

        bool visitUnionType(const UnionType& type) const override {
            return any_of(type.getElementTypes(), [&](const Type* cur) { return visit(*cur); });
        }

        bool visitType(const Type& /*type*/) const override {
            return false;
        }
    };

    return visitor(a).visit(b);
}
}  // namespace

/* generate unique type qualifier string for a type */
std::string getTypeQualifier(const Type& type) {
    struct visitor : public VisitOnceTypeVisitor<std::string> {
        std::string visitUnionType(const UnionType& type) const override {
            std::string str = visitType(type);
            str += "[";
            bool first = true;
            for (auto unionType : type.getElementTypes()) {
                if (first) {
                    first = false;
                } else {
                    str += ",";
                }
                str += visit(*unionType);
            }
            str += "]";
            return str;
        }

        std::string visitRecordType(const RecordType& type) const override {
            std::string str = visitType(type);
            str += "{";
            bool first = true;
            for (auto field : type.getFields()) {
                if (first) {
                    first = false;
                } else {
                    str += ",";
                }
                str += field.name;
                str += "#";
                str += visit(field.type);
            }
            str += "}";
            return str;
        }

        std::string visitType(const Type& type) const override {
            std::string str;

            switch (getTypeAttribute(type)) {
                case TypeAttribute::Signed:
                    str.append("i");
                    break;
                case TypeAttribute::Unsigned:
                    str.append("u");
                    break;
                case TypeAttribute::Float:
                    str.append("f");
                    break;
                case TypeAttribute::Symbol:
                    str.append("s");
                    break;
                case TypeAttribute::Record:
                    str.append("r");
                    break;
            }
            str.append(":");
            str.append(toString(type.getName()));
            seen[&type] = str;
            return str;
        }
    };

    return visitor().visit(type);
}

bool hasSignedType(const TypeSet& types) {
    return types.isAll() || any_of(types, (bool (*)(const Type&)) & isNumberType);
}

bool hasUnsignedType(const TypeSet& types) {
    return types.isAll() || any_of(types, (bool (*)(const Type&)) & isUnsignedType);
}

bool hasFloatType(const TypeSet& types) {
    return types.isAll() || any_of(types, (bool (*)(const Type&)) & isFloatType);
}

bool isFloatType(const Type& type) {
    return isOfRootType(type, type.getTypeEnvironment().getConstantType(TypeAttribute::Float));
}

bool isFloatType(const TypeSet& s) {
    return !s.empty() && !s.isAll() && all_of(s, (bool (*)(const Type&)) & isFloatType);
}

bool isNumberType(const Type& type) {
    return isOfRootType(type, type.getTypeEnvironment().getConstantType(TypeAttribute::Signed));
}

bool isNumberType(const TypeSet& s) {
    return !s.empty() && !s.isAll() && all_of(s, (bool (*)(const Type&)) & isNumberType);
}

bool isUnsignedType(const Type& type) {
    return isOfRootType(type, type.getTypeEnvironment().getConstantType(TypeAttribute::Unsigned));
}

bool isUnsignedType(const TypeSet& s) {
    return !s.empty() && !s.isAll() && all_of(s, (bool (*)(const Type&)) & isUnsignedType);
}

bool isSymbolType(const Type& type) {
    return isOfRootType(type, type.getTypeEnvironment().getConstantType(TypeAttribute::Symbol));
}

bool isSymbolType(const TypeSet& s) {
    return !s.empty() && !s.isAll() && all_of(s, (bool (*)(const Type&)) & isSymbolType);
}

bool isRecordType(const Type& type) {
    return isA<RecordType>(type);
}

bool isRecordType(const TypeSet& s) {
    return !s.empty() && !s.isAll() && all_of(s, isA<RecordType>);
}

bool isRecursiveType(const Type& type) {
    struct visitor : public VisitOnceTypeVisitor<bool> {
        const Type& trg;
        explicit visitor(const Type& trg) : trg(trg) {}
        bool visit(const Type& type) const override {
            if (trg == type) {
                return true;
            }
            return VisitOnceTypeVisitor<bool>::visit(type);
        }
        bool visitUnionType(const UnionType& type) const override {
            auto reachesTrg = [&](const Type* cur) { return this->visit(*cur); };
            return any_of(type.getElementTypes(), reachesTrg);
        }
        bool visitRecordType(const RecordType& type) const override {
            auto reachesTrg = [&](const RecordType::Field& cur) { return this->visit(cur.type); };
            return any_of(type.getFields(), reachesTrg);
        }
    };

    // record types are recursive if they contain themselves
    if (const auto* r = dynamic_cast<const RecordType*>(&type)) {
        auto reachesOrigin = visitor(type);
        return any_of(r->getFields(),
                [&](const RecordType::Field& field) -> bool { return reachesOrigin(field.type); });
    }

    return false;
}

bool isSubtypeOf(const Type& a, const Type& b) {
    // make sure they are both in the same environment
    auto& environment = a.getTypeEnvironment();
    assert(environment.isType(a) && environment.isType(b));

    // sub-type relation is reflexive
    if (a == b) {
        return true;
    }

    // check for subtypes.
    if (isOfRootType(a, b)) {
        return true;
    }

    // check primitive type chains
    // if (isA<SubsetType>(b)) {
    //     if (isSubtypeOf(a, as<SubsetType>(b).getBaseType())) {
    //         return true;
    //     }
    // }

    // next - if b is a union type
    if (isRecursiveType(a) || isRecursiveType(b)) {
        return false;
    }

    if (isA<UnionType>(b)) {
        if (!isA<UnionType>(a)) {
            return any_of(as<UnionType>(b).getElementTypes(),
                    [&](const Type* type) { return isSubtypeOf(a, *type); });
        } else {
            return all_of(as<UnionType>(a).getElementTypes(),
                    [&](const Type* type) { return isSubtypeOf(*type, b); });
        }
    }

    return false;
}

bool areSubtypesOf(const TypeSet& s, const Type& b) {
    return all_of(s, [&](const Type& t) { return isSubtypeOf(t, b); });
}

void TypeEnvironment::print(std::ostream& out) const {
    out << "Types:\n";
    for (const auto& cur : types) {
        out << "\t" << *cur.second << "\n";
    }
}

TypeSet getLeastCommonSupertypes(const Type& a, const Type& b) {
    // make sure they are in the same type environment
    assert(a.getTypeEnvironment().isType(a) && a.getTypeEnvironment().isType(b));

    // supertype relation is reflexive.
    if (a == b) {
        return TypeSet(a);
    }

    // equally simple - check whether one is a sub-type of the other
    if (isSubtypeOf(a, b)) {
        return TypeSet(b);
    }
    if (isSubtypeOf(b, a)) {
        return TypeSet(a);
    }

    // Compute all types t, such that a <: t and b <: t.
    TypeSet superTypes;
    TypeSet all = a.getTypeEnvironment().getAllTypes();
    for (const Type& type : all) {
        if (isSubtypeOf(a, type) && isSubtypeOf(b, type)) {
            superTypes.insert(type);
        }
    }

    // Find all T such that, such that for any t, t <: T implies t = T.
    TypeSet leastSuperType;
    for (const Type& type : superTypes) {
        bool isLeast = all_of(superTypes, [&](const Type& t) { return !isSubtypeOf(t, type) || t == type; });
        if (isLeast) {
            leastSuperType.insert(type);
        }
    }

    return leastSuperType;
}

TypeSet getLeastCommonSupertypes(const TypeSet& set) {
    // handle the empty set
    if (set.empty()) {
        return set;
    }

    // handle the all set => empty set (since no common super-type)
    if (set.isAll()) {
        return TypeSet();
    }

    TypeSet res;
    auto it = set.begin();
    res.insert(*it);
    ++it;

    // refine sub-set step by step
    for (; it != set.end(); ++it) {
        TypeSet tmp;
        for (const Type& cur : res) {
            tmp.insert(getLeastCommonSupertypes(cur, *it));
        }
        res = tmp;
    }

    // done
    return res;
}

// pairwise
TypeSet getLeastCommonSupertypes(const TypeSet& a, const TypeSet& b) {
    // special cases
    if (a.empty()) {
        return a;
    }
    if (b.empty()) {
        return b;
    }

    if (a.isAll()) {
        return b;
    }
    if (b.isAll()) {
        return a;
    }

    // compute pairwise least common super types
    TypeSet res;
    for (const Type& x : a) {
        for (const Type& y : b) {
            res.insert(getLeastCommonSupertypes(x, y));
        }
    }
    return res;
}

TypeSet getGreatestCommonSubtypes(const Type& a, const Type& b) {
    assert(a.getTypeEnvironment().isType(a) && a.getTypeEnvironment().isType(b) &&
            "Types must be in the same type environment");

    // subtype is reflexive.
    if (a == b) {
        return TypeSet(a);
    }

    if (isSubtypeOf(a, b)) {
        return TypeSet(a);
    }
    if (isSubtypeOf(b, a)) {
        return TypeSet(b);
    }

    // last option: if both are unions with common sub-types
    TypeSet res;
    if (isA<UnionType>(a) && isA<UnionType>(b)) {
        // collect common sub-types of union types
        struct collector : public TypeVisitor<void> {
            const Type& b;
            TypeSet& res;
            collector(const Type& b, TypeSet& res) : b(b), res(res) {}

            void visit(const Type& type) const override {
                if (isSubtypeOf(type, b)) {
                    res.insert(type);
                } else {
                    TypeVisitor<void>::visit(type);
                }
            }
            void visitUnionType(const UnionType& type) const override {
                for (const auto& cur : type.getElementTypes()) {
                    visit(*cur);
                }
            }
        };

        // collect all common sub-types
        collector(b, res).visit(a);
    }

    // otherwise there is no common super type
    return res;
}

TypeSet getGreatestCommonSubtypes(const TypeSet& set) {
    // handle the empty set
    if (set.empty()) {
        return set;
    }

    // handle the all set => empty set (since no common sub-type)
    if (set.isAll()) {
        return TypeSet();
    }

    TypeSet res;
    auto it = set.begin();
    res.insert(*it);
    ++it;

    // refine sub-set step by step
    for (; it != set.end(); ++it) {
        TypeSet tmp;
        for (const Type& cur : res) {
            tmp.insert(getGreatestCommonSubtypes(cur, *it));
        }
        res = tmp;
    }

    // done
    return res;
}

TypeSet getGreatestCommonSubtypes(const TypeSet& a, const TypeSet& b) {
    // special cases
    if (a.empty()) {
        return a;
    }
    if (b.empty()) {
        return b;
    }

    if (a.isAll()) {
        return b;
    }
    if (b.isAll()) {
        return a;
    }

    // compute pairwise greatest common sub types
    TypeSet res;
    for (const Type& x : a) {
        for (const Type& y : b) {
            res.insert(getGreatestCommonSubtypes(x, y));
        }
    }
    return res;
}

}  // end of namespace souffle
