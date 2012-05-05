
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// A0.il --> A0.dat
//
// This is the core metadata test assembly.  It defines simple types that validate all of the basic
// metadata rules from Partition II of ECMA-335 5ed/2010.  It contains only metadata and has no
// executable code.  The test assemblies A1 and A2 contain additional tests that verify cross-assembly
// type and member resolution (since these tests necessarily require multiple assemblies).
//
// Note that we don't test the entire CIL syntax; we only define tests that affect the metadata that
// is produced by ilasm.





.assembly extern mscorlib  { .ver 4:0:0:0 }

.assembly A0
{
    .ver 1:0:0:0
}

.module     A0
.corflags   0x00000001 // ILONLY





// II.10.1.1 VISIBILITY AND ACCESSIBILITY ATTRIBUTES

.class private         QTrivialPrivateClass         { }
.class public          QTrivialPublicClass          { }

.class QTrivialVisibilityNestingClass
{
    .class nested assembly    QTrivialNestedAssemblyClass    { }
    .class nested famandassem QTrivialNestedFamAndAssemClass { }
    .class nested family      QTrivialNestedFamilyClass      { }
    .class nested famorassem  QTrivialNestedFamOrAssemClass  { }
    .class nested private     QTrivialNestedPrivateClass     { }
    .class nested public      QTrivialNestedPublicClass      { }
}





// II.10.1.2 TYPE LAYOUT ATTRIBUTES

.class auto       QTrivialAutoClass       { }
.class explicit   QTrivialExplicitClass   { }
.class sequential QTrivialSequentialClass { }





// II.10.1.3 TYPE SEMANTICS ATTRIBUTES

.class           QTrivialClass          { }
.class interface QTrivialInterfaceClass { }

.class sealed QTrivialValueTypeClass extends [mscorlib]System.ValueType { }

.class sealed QTrivialEnumClass      extends [mscorlib]System.Enum
{
    .field public specialname rtspecialname int32 value__
    //.field public static literal valuetype QTrivialEnumClass QTrivialEnumClassEnumerator = int32(0x00000000)
}





// II.10.1.4 INHERITANCE ATTRIBUTES

.class abstract        QTrivialAbstractClass       { }
.class sealed          QTrivialSealedClass         { }
.class abstract sealed QTrivialAbstractSealedClass { }





// II.10.1.5 INTEROPERATION ATTRIBUTES

.class ansi     QTrivialAnsiClass     { }
.class autochar QTrivialAutoCharClass { }
.class unicode  QTrivialUnicodeClass  { }





// II.10.1.6 SPECIAL HANDLING ATTRIBUTES

.class beforefieldinit           QTrivialBeforeFieldInitClass { }
.class serializable              QTrivialSerializableClass    { }
.class specialname               QTrivialSpecialNameClass     { }
.class specialname rtspecialname QTrivialRTSpecialNameClass   { }

.class beforefieldinit serializable                           QTrivialSpecialHandlingClass00 { }
.class beforefieldinit                            specialname QTrivialSpecialHandlingClass01 { }
.class beforefieldinit              rtspecialname specialname QTrivialSpecialHandlingClass02 { }
.class beforefieldinit serializable               specialname QTrivialSpecialHandlingClass03 { }
.class beforefieldinit serializable rtspecialname specialname QTrivialSpecialHandlingClass04 { }
.class                 serializable               specialname QTrivialSpecialHandlingClass06 { }
.class                 serializable rtspecialname specialname QTrivialSpecialHandlingClass07 { }





// II.10.1.7 GENERIC PARAMETERS
// TODO LEFT OFF HERE

.class QTrivialGeneric`1<T>    { }
.class QTrivialGeneric`2<T, U> { }









// TRIVIAL INHERITANCE CLASS DEFINITIONS (II.10.) -- These class definitions test trivial inheritance
// (extends) and interface implementation (implements) scenarios.

.class QExtendsSystemObject extends [mscorlib]System.Object { }

.class QExtendsB0 { }
.class QExtendsB1 extends QExtendsB0 { }
.class QExtendsB2 extends QExtendsB1 { }

.class interface QExtendsI0 { }
.class interface QExtendsI1 implements QExtendsI0 { }
.class interface QExtendsI2 { }

.class QExtendsD0 extends QExtendsB0 implements QExtendsI0 { }
.class QExtendsD1 extends QExtendsB1 implements QExtendsI1 { }
.class QExtendsD2 extends QExtendsB2 implements QExtendsI2 { }

.class QExtendsD3 extends QExtendsB0 implements QExtendsI0, QExtendsI1 { }
.class QExtendsD4 extends QExtendsB0 implements QExtendsI0, QExtendsI2 { }
.class QExtendsD5 extends QExtendsB0 implements QExtendsI1, QExtendsI2 { }
