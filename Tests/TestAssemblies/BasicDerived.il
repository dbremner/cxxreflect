
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

.assembly extern mscorlib  { .ver 4:0:0:0 }
.assembly extern BasicBase { .ver 1:0:0:0 }

.assembly BasicDerived
{
    .ver 1:0:0:0
}

.module     BasicDerived
.corflags   0x00000001 // ILONLY

.class  public auto ansi BasicDerived.D0
        extends [BasicBase]BasicBase.B0
{
}
