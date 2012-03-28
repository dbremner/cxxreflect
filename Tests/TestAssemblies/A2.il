//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

.assembly extern mscorlib  { .ver 4:0:0:0 }
.assembly extern A1        { .ver 1:0:0:0 }

.assembly A2
{
    .ver 1:0:0:0
}

.module     A2
.corflags   0x00000001 // ILONLY
