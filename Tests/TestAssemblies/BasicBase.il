
//               Copyright James P. McNellis (james@jamesmcnellis.com) 2011 - 2012.               //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This file defines a fake system assembly for use with C++ projects targeting the Windows Runtime.

.assembly extern mscorlib { .ver 4:0:0:0 }

.assembly BasicBase
{
    .ver 1:0:0:0
}

.module     BasicBase
.corflags   0x00000001 // ILONLY

.class  public auto ansi BasicBase.B0
        extends [mscorlib]System.Object
{
}
