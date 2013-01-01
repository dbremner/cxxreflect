
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //





.assembly extern mscorlib { .ver 4:0:0:0 }

.assembly beta
{
    .ver 1:2:3:4
}

.module     beta
.corflags   0x00000001 // ILONLY






.class private auto ansi beforefieldinit MPropertySimple extends [mscorlib]System.Object
{
    .method public hidebysig specialname instance int32 get_RW()              { ret; }
    .method public hidebysig specialname instance void  set_RW(int32 'value') { ret; }
    .method public hidebysig specialname instance int32 get_R ()              { ret; }
    .method public hidebysig specialname instance void  set_W (int32 'value') { ret; }

    .property instance int32 RW()
    {
        .get instance int32 MPropertySimple::get_RW()
        .set instance void  MPropertySimple::set_RW(int32)
    }

    .property instance int32 R()
    {
        .get instance int32 MPropertySimple::get_R()
    }

    .property instance int32 W()
    {
        .set instance void MPropertySimple::set_W(int32)
    } 
}





.class public auto ansi beforefieldinit ZArgument extends [mscorlib]System.Object
{
    .method public hidebysig specialname rtspecialname instance void .ctor() { ret; }
}

.class public auto ansi beforefieldinit ZReturn extends [mscorlib]System.Object
{
    .method public hidebysig specialname rtspecialname instance void .ctor() { ret; }
}

.class interface public abstract auto ansi ZBaseInterface
{
    .method public hidebysig newslot abstract virtual instance void F() { }
    .method public hidebysig newslot abstract virtual instance bool G(class ZArgument a) { }
}

.class public auto ansi beforefieldinit ZBase
    extends [mscorlib]System.Object
    implements ZBaseInterface
{
  .method public hidebysig specialname rtspecialname instance void .ctor() { ret; }

  .method public hidebysig newslot virtual final instance void F() { ret; }
  .method public hidebysig newslot virtual final instance bool G(class ZArgument b) { ret; }

  .method public hidebysig newslot virtual instance void M() { ret; }
  .method public hidebysig newslot virtual instance bool N() { ret; }

  .method public hidebysig instance void O() { ret; }

  .method public hidebysig instance class ZReturn 
          P(class ZArgument x,
            class ZArgument y,
            class ZArgument z) cil managed
  {
    ret;
  }
}
