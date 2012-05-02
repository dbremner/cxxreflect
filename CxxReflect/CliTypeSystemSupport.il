
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This file defines replacement types used to support the metadata loader when loading metadata for
// the CLI type system.

.assembly extern mscorlib  { .ver 4:0:0:0 }

.assembly CliTypeSystemSupport
{
    .ver 1:0:0:0
}

.module     CliTypeSystemSupport.dll
.corflags   0x00000001 // ILONLY

.class  public auto ansi CxxReflect.ClrReplacements.GenericArray`1<T>
        extends [mscorlib]System.Array
{

}
