/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file type_system_test.h
 *
 * Tests souffle's type system operations.
 *
 ***********************************************************************/

#include "TypeSystem.h"
#include "Util.h"
#include "test.h"

namespace souffle::test {

// TEST(TypeSystem, Basic) {
//     TypeEnvironment env;

//     auto& A = env.createSubsetType("A", TypeAttribute::Signed);
//     auto& B = env.createSubsetType("B", TypeAttribute::Symbol);

//     auto& U = env.createType<UnionType>("U");
//     U.add(A);
//     U.add(B);

//     auto& R = env.createType<RecordType>("R");
//     R.add("a", A);
//     R.add("b", B);

//     EXPECT_EQ("A <: number", toString(A));
//     EXPECT_EQ("B <: symbol", toString(B));

//     EXPECT_EQ("U = A | B", toString(U));
//     EXPECT_EQ("R = ( a : A , b : B )", toString(R));

// }

// TEST(TypeSystem, isNumberType) {
//     TypeEnvironment env;

//     auto& N = env.getType("number");

//     auto& A = env.createSubsetType("A", TypeAttribute::Signed);
//     auto& B = env.createSubsetType("B", TypeAttribute::Signed);

//     auto& C = env.createSubsetType("C", TypeAttribute::Symbol);

//     EXPECT_TRUE(isNumberType(N));
//     EXPECT_TRUE(isNumberType(A));
//     EXPECT_TRUE(isNumberType(B));
//     EXPECT_TRUE(isSymbolType(C));

//     EXPECT_FALSE(isSymbolType(N));
//     EXPECT_FALSE(isSymbolType(A));
//     EXPECT_FALSE(isSymbolType(B));
//     EXPECT_FALSE(isNumberType(C));

//     // check the union type
//     {
//         auto& U = env.createType<UnionType>("U");
//         EXPECT_FALSE(isNumberType(U));
//         EXPECT_FALSE(isSymbolType(U));
//         U.add(A);
//         EXPECT_TRUE(isNumberType(U));
//         EXPECT_FALSE(isSymbolType(U));
//         U.add(B);
//         EXPECT_TRUE(isNumberType(U));
//         EXPECT_FALSE(isSymbolType(U));
//         U.add(C);
//         EXPECT_FALSE(isNumberType(U));
//         EXPECT_FALSE(isSymbolType(U));
//     }

//     // make type recursive
//     {
//         auto& U = env.createType<UnionType>("U2");

//         EXPECT_FALSE(isNumberType(U));
//         U.add(A);
//         EXPECT_TRUE(isNumberType(U));

//         U.add(U);
//         EXPECT_FALSE(isNumberType(U));
//     }
// }

// TEST(TypeSystem, isRecursiveType) {
//     TypeEnvironment env;

//     auto& A = env.createSubsetType("A", TypeAttribute::Signed);
//     auto& B = env.createSubsetType("B", TypeAttribute::Signed);

//     auto& U = env.createType<UnionType>("U");
//     auto& R = env.createType<RecordType>("R");
//     R.add("h", A);
//     R.add("t", U);

//     U.add(R);  // a not-really recursive union type

//     // primitive types are never recursive
//     EXPECT_FALSE(isRecursiveType(A)) << A;

//     // neither are union types
//     EXPECT_FALSE(isRecursiveType(U)) << U;

//     // but R = [ h : A , t : U = R ] is
//     EXPECT_TRUE(isRecursiveType(R)) << R;

//     // create a real recursive type
//     auto& List = env.createType<RecordType>("List");
//     EXPECT_FALSE(isRecursiveType(List));
//     List.add("head", A);
//     EXPECT_FALSE(isRecursiveType(List));
//     List.add("tail", List);
//     EXPECT_TRUE(isRecursiveType(List));

//     // a mutual recursive type
//     auto& E = env.createType<RecordType>("E");
//     auto& O = env.createType<RecordType>("O");

//     EXPECT_FALSE(isRecursiveType(E));
//     EXPECT_FALSE(isRecursiveType(O));

//     E.add("head", A);
//     E.add("tail", O);

//     EXPECT_FALSE(isRecursiveType(E));
//     EXPECT_FALSE(isRecursiveType(O));

//     O.add("head", B);
//     O.add("tail", E);

//     EXPECT_TRUE(isRecursiveType(E));
//     EXPECT_TRUE(isRecursiveType(O));
// }

// bool isNotSubtypeOf(const Type& a, const Type& b) {
//     return !isSubtypeOf(a, b);
// }

// TEST(TypeSystem, isSubtypeOf_Basic) {
//     TypeEnvironment env;

//     // start with the two predefined types

//     auto& N = env.getType("number");
//     auto& S = env.getType("symbol");

//     EXPECT_PRED2(isSubtypeOf, N, N);
//     EXPECT_PRED2(isSubtypeOf, S, S);

//     EXPECT_PRED2(isNotSubtypeOf, N, S);
//     EXPECT_PRED2(isNotSubtypeOf, S, N);

//     // check primitive type

//     auto& A = env.createSubsetType("A", TypeAttribute::Signed);
//     auto& B = env.createSubsetType("B", TypeAttribute::Signed);

//     EXPECT_PRED2(isSubtypeOf, A, A);
//     EXPECT_PRED2(isSubtypeOf, B, B);

//     EXPECT_PRED2(isNotSubtypeOf, A, B);
//     EXPECT_PRED2(isNotSubtypeOf, B, A);

//     EXPECT_PRED2(isSubtypeOf, A, N);
//     EXPECT_PRED2(isSubtypeOf, B, N);

//     EXPECT_PRED2(isNotSubtypeOf, A, S);
//     EXPECT_PRED2(isNotSubtypeOf, B, S);

//     // check union types

//     auto& U = env.createType<UnionType>("U");
//     U.add(A);
//     U.add(B);

//     EXPECT_PRED2(isSubtypeOf, U, U);
//     EXPECT_PRED2(isSubtypeOf, A, U);
//     EXPECT_PRED2(isSubtypeOf, B, U);
//     EXPECT_PRED2(isSubtypeOf, U, N);

//     EXPECT_PRED2(isNotSubtypeOf, U, A);
//     EXPECT_PRED2(isNotSubtypeOf, U, B);
//     EXPECT_PRED2(isNotSubtypeOf, N, U);

//     auto& V = env.createType<UnionType>("V");
//     // EXPECT_PRED2(isNotSubtypeOf, V, U);
//     EXPECT_PRED2(isNotSubtypeOf, U, V);

//     V.add(A);
//     // EXPECT_PRED2(isNotSubtypeOf, V, U);
//     EXPECT_PRED2(isNotSubtypeOf, U, V);

//     V.add(B);
//     // EXPECT_PRED2(isNotSubtypeOf, V, U);
//     EXPECT_PRED2(isNotSubtypeOf, U, V);

//     V.add(U);
//     // EXPECT_PRED2(isNotSubtypeOf, V, U);
//     EXPECT_PRED2(isSubtypeOf, U, V);
// }

// TEST(TypeSystem, isSubtypeOf_Records) {
//     TypeEnvironment env;

//     auto& A = env.createSubsetType("A", TypeAttribute::Signed);
//     auto& B = env.createSubsetType("B", TypeAttribute::Signed);

//     auto& R1 = env.createType<RecordType>("R1");
//     auto& R2 = env.createType<RecordType>("R2");

//     EXPECT_FALSE(isSubtypeOf(R1, R2));
//     EXPECT_FALSE(isSubtypeOf(R2, R1));

//     R1.add("a", A);
//     R2.add("b", B);
//     EXPECT_FALSE(isSubtypeOf(R1, R2));
//     EXPECT_FALSE(isSubtypeOf(R2, R1));
// }

// TEST(TypeSystem, GreatestCommonSubtype) {
//     TypeEnvironment env;

//     auto& N = env.getType("number");

//     auto& A = env.createSubsetType("A", TypeAttribute::Signed);
//     auto& B = env.createSubsetType("B", TypeAttribute::Signed);
//     auto& C = env.createSubsetType("C", TypeAttribute::Symbol);

//     EXPECT_EQ("{number}", toString(getGreatestCommonSubtypes(N, N)));

//     EXPECT_EQ("{A}", toString(getGreatestCommonSubtypes(A, A)));
//     EXPECT_EQ("{B}", toString(getGreatestCommonSubtypes(B, B)));
//     EXPECT_EQ("{C}", toString(getGreatestCommonSubtypes(C, C)));

//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(A, B)));
//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(A, C)));
//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(B, C)));

//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(A, B, C)));

//     EXPECT_EQ("{A}", toString(getGreatestCommonSubtypes(A, N)));
//     EXPECT_EQ("{A}", toString(getGreatestCommonSubtypes(N, A)));

//     EXPECT_EQ("{B}", toString(getGreatestCommonSubtypes(B, N)));
//     EXPECT_EQ("{B}", toString(getGreatestCommonSubtypes(N, B)));

//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(C, N)));
//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(N, C)));

//     // bring in unions

//     auto& U = env.createType<UnionType>("U");
//     auto& S = env.createType<UnionType>("S");

//     U.add(A);
//     // EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(U, S)));

//     S.add(A);
//     EXPECT_EQ("{A}", toString(getGreatestCommonSubtypes(U, S)));

//     U.add(B);
//     EXPECT_EQ("{A}", toString(getGreatestCommonSubtypes(U, S)));
//     EXPECT_EQ("{A}", toString(getGreatestCommonSubtypes(U, S, N)));

//     S.add(B);
//     EXPECT_EQ("{A,B}", toString(getGreatestCommonSubtypes(U, S)));
//     EXPECT_EQ("{A,B}", toString(getGreatestCommonSubtypes(U, S, N)));

//     // bring in a union of unions
//     auto& R = env.createType<UnionType>("R");

//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(U, R)));
//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(S, R)));

//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(U, R, N)));
//     EXPECT_EQ("{}", toString(getGreatestCommonSubtypes(S, R, N)));

//     R.add(U);

//     EXPECT_EQ("{U}", toString(getGreatestCommonSubtypes(U, R)));
//     EXPECT_EQ("{A,B}", toString(getGreatestCommonSubtypes(S, R)));

//     EXPECT_EQ("{U}", toString(getGreatestCommonSubtypes(U, R, N)));
//     EXPECT_EQ("{A,B}", toString(getGreatestCommonSubtypes(S, R, N)));

//     R.add(S);

//     EXPECT_EQ("{U}", toString(getGreatestCommonSubtypes(U, R)));
//     EXPECT_EQ("{S}", toString(getGreatestCommonSubtypes(S, R)));

//     EXPECT_EQ("{U}", toString(getGreatestCommonSubtypes(U, R, N)));
//     EXPECT_EQ("{S}", toString(getGreatestCommonSubtypes(S, R, N)));
// }

// TEST(TypeSystem, LeastCommonSupertype) {
//     TypeEnvironment env;

//     auto& A = env.createSubsetType("A", TypeAttribute::Signed);
//     auto& B = env.createSubsetType("B", TypeAttribute::Signed);
//     auto& C = env.createSubsetType("C", TypeAttribute::Symbol);
//     auto& D = env.createSubsetType("D", TypeAttribute::Symbol);

//     auto& U = env.createType<UnionType>("U");
//     U.add(A);

//     auto& V = env.createType<UnionType>("V");
//     V.add(U);
//     V.add(B);

//     auto& W = env.createType<UnionType>("W");
//     W.add(V);
//     W.add(C);

//     EXPECT_TRUE(isSubtypeOf(A, env.getType("number")));
//     EXPECT_TRUE(isSubtypeOf(U, env.getType("number")));
//     EXPECT_TRUE(isSubtypeOf(V, env.getType("number")));

//     EXPECT_EQ("{}", toString(getLeastCommonSupertypes()));
//     EXPECT_EQ("{A}", toString(getLeastCommonSupertypes(A)));
//     EXPECT_EQ("{V}", toString(getLeastCommonSupertypes(A, B)));
//     EXPECT_EQ("{W}", toString(getLeastCommonSupertypes(A, B, C)));
//     EXPECT_EQ("{}", toString(getLeastCommonSupertypes(A, B, C, D)));

//     EXPECT_EQ("{symbol}", toString(getLeastCommonSupertypes(C, D)));
//     EXPECT_EQ("{}", toString(getLeastCommonSupertypes(A, D)));

//     EXPECT_EQ("{V}", toString(getLeastCommonSupertypes(U, B)));
// }

// TEST(TypeSystem, MultipleLeastCommonSupertype) {
//     TypeEnvironment env;

//     auto& A = env.createSubsetType("A", TypeAttribute::Signed);
//     auto& B = env.createSubsetType("B", TypeAttribute::Signed);

//     auto& U = env.createType<UnionType>("U");
//     U.add(A);
//     U.add(B);

//     auto& V = env.createType<UnionType>("V");
//     V.add(A);
//     V.add(B);

//     EXPECT_EQ("{U,V}", toString(getLeastCommonSupertypes(A, B)));
// }

}  // namespace souffle::test
