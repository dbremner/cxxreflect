//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This file defines a fake system assembly for use with C++ projects targeting the Windows Runtime.

.assembly extern mscorlib
{
    .ver 255:255:255:255
}

.module     CxxReflectPlatform.winmd
.corflags   0x00000001 // ILONLY

.class  public auto ansi System.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }

    .method public hidebysig virtual instance
            class System.Type GetType() runtime managed { }

    .method public hidebysig virtual instance
            bool Equals(object obj) runtime managed { }

    .method public hidebysig virtual instance
            int32 GetHashCode() runtime managed { }

    .method public hidebysig virtual instance
            string ToString() runtime managed { }

    .method public hidebysig static
            bool ReferenceEquals(object obj1, object obj2) runtime managed { }

    .method public hidebysig static
            bool ReferenceEquals(string obj1, string obj2) runtime managed { }

    .method family hidebysig newslot instance
            void Finalize() runtime managed { }
}

.class  public auto ansi System.Type
        extends System.Object
        // TODO IMPLEMENTS
{
}
