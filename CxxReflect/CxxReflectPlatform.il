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

.class public auto ansi System.ValueType extends System.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

.class public auto ansi System.Attribute extends System.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

.class public auto ansi System.FlagsAttribute extends System.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

.class public auto ansi System.AccessDeniedException extends System.Object { }
.class public auto ansi System.Boolean extends System.Object { }
.class public auto ansi System.COMException extends System.Object { }
.class public auto ansi System.CallbackContext extends System.Object { }
.class public auto ansi System.ChangedStateException extends System.Object { }
.class public auto ansi System.ClassNotRegisteredException extends System.Object { }
.class public auto ansi System.Delegate extends System.Object { }
.class public auto ansi System.DisconnectedException extends System.Object { }
.class public auto ansi System.Enum extends System.Object { }
.class public auto ansi System.Exception extends System.Object { }
.class public auto ansi System.FailureException extends System.Object { }
.class public auto ansi System.Guid extends System.Object { }
.class public auto ansi System.IBoxArray`1<T> extends System.Object { }
.class public auto ansi System.IBox`1<T> extends System.Object { }
.class public auto ansi System.IDisposable extends System.Object { }
.class public auto ansi System.IValueType extends System.Object { }
.class public auto ansi System.IntPtr extends System.Object { }
.class public auto ansi System.InvalidArgumentException extends System.Object { }
.class public auto ansi System.InvalidCastException extends System.Object { }
.class public auto ansi System.MTAThreadAttribute extends System.Object { }
.class public auto ansi System.MulticastDelegate extends System.Object { }
.class public auto ansi System.NotImplementedException extends System.Object { }
.class public auto ansi System.NullReferenceException extends System.Object { }
.class public auto ansi System.OperationCancelledException extends System.Object { }
.class public auto ansi System.OutOfBoundsException extends System.Object { }
.class public auto ansi System.OutOfMemoryException extends System.Object { }
.class public auto ansi System.STAThreadAttribute extends System.Object { }
.class public auto ansi System.SizeT extends System.Object { }
.class public auto ansi System.TypeCode extends System.Object { }
.class public auto ansi System.UIntPtr extends System.Object { }
