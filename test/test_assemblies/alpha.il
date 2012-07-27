
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// alpha.il --> alpha.dat
//
// This is the core metadata test assembly.  It defines simple types that validate all of the basic
// metadata rules from Partition II of ECMA 335-2010.  It contains only metadata and has no
// executable code.  The test assemblies A1 and A2 contain additional tests that verify cross-assembly
// type and member resolution (since these tests necessarily require multiple assemblies).
//
// Note that we don't test the entire CIL syntax; we only define tests that affect the metadata that
// is produced by ilasm.





.assembly extern mscorlib  { .ver 4:0:0:0 }

.assembly alpha
{
    .ver 1:2:3:4
}

.module     alpha
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
    .field public static literal valuetype QTrivialEnumClass QTrivialEnumClassEnumerator = int32(0x00000000)
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
.class                 serializable               specialname QTrivialSpecialHandlingClass05 { }
.class                 serializable rtspecialname specialname QTrivialSpecialHandlingClass06 { }





// NOT IN SPECIFICATION

.class QTrivialComImport
{
    .custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) =
        ( 01 00 24 30 30 30 30 30 30 30 30 2D 30 30 30 30   // ..$00000000-0000
          2D 30 30 30 30 2D 30 30 30 30 2D 30 30 30 30 30   // -0000-0000-00000
          30 30 30 30 30 30 30 00 00 )                      // 0000000..
}

.class QTrivialComObject          extends [mscorlib]System.__ComObject        { }
.class QTrivialContextBoundObject extends [mscorlib]System.ContextBoundObject { }
.class QTrivialMarshalByRefObject extends [mscorlib]System.MarshalByRefObject { }





// TODO TODO TODO ???? TODO TODO TODO

.class QTrivialTypeMethodChecks
{
    .method private hidebysig instance void TestByRef(class     QTrivialPrivateClass          & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialPublicClass           & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialVisibilityNestingClass/QTrivialNestedAssemblyClass   & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialVisibilityNestingClass/QTrivialNestedFamAndAssemClass& x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialVisibilityNestingClass/QTrivialNestedFamilyClass     & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialVisibilityNestingClass/QTrivialNestedFamOrAssemClass & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialVisibilityNestingClass/QTrivialNestedPrivateClass    & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialVisibilityNestingClass/QTrivialNestedPublicClass     & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialClass                 & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialInterfaceClass        & x) { ret; }
    .method private hidebysig instance void TestByRef(valuetype QTrivialValueTypeClass        & x) { ret; }
    .method private hidebysig instance void TestByRef(valuetype QTrivialEnumClass             & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialAbstractClass         & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialSealedClass           & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialAnsiClass             & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialAutoCharClass         & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialUnicodeClass          & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialBeforeFieldInitClass  & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialSerializableClass     & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialSpecialNameClass      & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialRTSpecialNameClass    & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialComImport             & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialComObject             & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialContextBoundObject    & x) { ret; }
    .method private hidebysig instance void TestByRef(class     QTrivialMarshalByRefObject    & x) { ret; }

    .method private hidebysig instance void TestSimpleArray(class     QTrivialPrivateClass          [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialPublicClass           [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialVisibilityNestingClass/QTrivialNestedAssemblyClass   [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialVisibilityNestingClass/QTrivialNestedFamAndAssemClass[] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialVisibilityNestingClass/QTrivialNestedFamilyClass     [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialVisibilityNestingClass/QTrivialNestedFamOrAssemClass [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialVisibilityNestingClass/QTrivialNestedPrivateClass    [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialVisibilityNestingClass/QTrivialNestedPublicClass     [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialClass                 [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialInterfaceClass        [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(valuetype QTrivialValueTypeClass        [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(valuetype QTrivialEnumClass             [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialAbstractClass         [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialSealedClass           [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialAnsiClass             [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialAutoCharClass         [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialUnicodeClass          [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialBeforeFieldInitClass  [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialSerializableClass     [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialSpecialNameClass      [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialRTSpecialNameClass    [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialComImport             [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialComObject             [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialContextBoundObject    [] x) { ret; }
    .method private hidebysig instance void TestSimpleArray(class     QTrivialMarshalByRefObject    [] x) { ret; }

    .method private hidebysig instance void TestPointer(class     QTrivialPrivateClass          * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialPublicClass           * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialVisibilityNestingClass/QTrivialNestedAssemblyClass   * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialVisibilityNestingClass/QTrivialNestedFamAndAssemClass* x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialVisibilityNestingClass/QTrivialNestedFamilyClass     * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialVisibilityNestingClass/QTrivialNestedFamOrAssemClass * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialVisibilityNestingClass/QTrivialNestedPrivateClass    * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialVisibilityNestingClass/QTrivialNestedPublicClass     * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialClass                 * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialInterfaceClass        * x) { ret; }
    .method private hidebysig instance void TestPointer(valuetype QTrivialValueTypeClass        * x) { ret; }
    .method private hidebysig instance void TestPointer(valuetype QTrivialEnumClass             * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialAbstractClass         * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialSealedClass           * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialAnsiClass             * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialAutoCharClass         * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialUnicodeClass          * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialBeforeFieldInitClass  * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialSerializableClass     * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialSpecialNameClass      * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialRTSpecialNameClass    * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialComImport             * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialComObject             * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialContextBoundObject    * x) { ret; }
    .method private hidebysig instance void TestPointer(class     QTrivialMarshalByRefObject    * x) { ret; }
}





// II.10.1.2 TYPE LAYOUT ATTRIBUTES

.class auto       QTrivialAutoClass       { }
.class explicit   QTrivialExplicitClass   { }
.class sequential QTrivialSequentialClass { }




// II.10.1.7 GENERIC PARAMETERS
// TODO LEFT OFF HERE

.class QTrivialGeneric00`1<T>    { }
.class QTrivialGeneric00`2<T, U> { }

.class QTrivialGeneric01`2<T,U>
{
    .method public hidebysig instance void F(!T t, [out] !U& u) { ret; }

    .method private hidebysig class QTrivialGeneric00`1<class QTrivialGeneric01`2<string, string>&> G() { ret; }
}


.class QClassWithCustomModifiers
{
    .method public hidebysig instance void F(
        bool
            modopt([mscorlib]System.UInt32) modreq([mscorlib]System.UInt64) * 
            modopt([mscorlib]System.Int32)  modreq([mscorlib]System.Int64)  arg)
    {
        ret;
    } 
}






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
