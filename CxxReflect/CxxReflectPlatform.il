//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This file defines a fake Platform assembly for use with C++ projects targeting the Windows Runtime.

.assembly CxxReflectPlatform
{
    .ver 1:0:0:0
}

.module     CxxReflectPlatform.winmd
.corflags   0x00000001 // ILONLY

.class  public auto ansi Platform.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }

    .method public hidebysig virtual instance
            class Platform.Type GetType() runtime managed { }

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

.class  public auto ansi Platform.Type
        extends class Platform.Object
        // TODO IMPLEMENTS
{
}

.class public auto ansi Platform.ValueType extends Platform.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

.class public auto ansi Platform.Attribute extends Platform.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

.class public auto ansi Platform.FlagsAttribute extends Platform.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

// The value type fundamental types
.class public auto ansi Platform.Boolean        extends Platform.ValueType { }
.class public auto ansi Platform.Char           extends Platform.ValueType { }
.class public auto ansi Platform.SByte          extends Platform.ValueType { }
.class public auto ansi Platform.Byte           extends Platform.ValueType { }
.class public auto ansi Platform.Int16          extends Platform.ValueType { }
.class public auto ansi Platform.UInt16         extends Platform.ValueType { }
.class public auto ansi Platform.Int32          extends Platform.ValueType { }
.class public auto ansi Platform.UInt32         extends Platform.ValueType { }
.class public auto ansi Platform.Int64          extends Platform.ValueType { }
.class public auto ansi Platform.UInt64         extends Platform.ValueType { }
.class public auto ansi Platform.Single         extends Platform.ValueType { }
.class public auto ansi Platform.Double         extends Platform.ValueType { }
.class public auto ansi Platform.IntPtr         extends Platform.ValueType { }
.class public auto ansi Platform.UIntPtr        extends Platform.ValueType { }
.class public auto ansi Platform.Void           extends Platform.ValueType { }
.class public auto ansi Platform.TypedReference extends Platform.ValueType { }

.class public auto ansi Platform.AccessDeniedException extends Platform.Object { }
.class public auto ansi Platform.Boolean extends Platform.Object { }
.class public auto ansi Platform.COMException extends Platform.Object { }
.class public auto ansi Platform.CallbackContext extends Platform.Object { }
.class public auto ansi Platform.ChangedStateException extends Platform.Object { }
.class public auto ansi Platform.ClassNotRegisteredException extends Platform.Object { }
.class public auto ansi Platform.Delegate extends Platform.Object { }
.class public auto ansi Platform.DisconnectedException extends Platform.Object { }
.class public auto ansi Platform.Enum extends Platform.Object { }
.class public auto ansi Platform.Exception extends Platform.Object { }
.class public auto ansi Platform.FailureException extends Platform.Object { }
.class public auto ansi Platform.Guid extends Platform.Object { }
.class public auto ansi Platform.IBoxArray`1<T> extends Platform.Object { }
.class public auto ansi Platform.IBox`1<T> extends Platform.Object { }
.class public auto ansi Platform.IDisposable extends Platform.Object { }
.class public auto ansi Platform.IValueType extends Platform.Object { }
.class public auto ansi Platform.InvalidArgumentException extends Platform.Object { }
.class public auto ansi Platform.InvalidCastException extends Platform.Object { }
.class public auto ansi Platform.MTAThreadAttribute extends Platform.Object { }
.class public auto ansi Platform.MulticastDelegate extends Platform.Object { }
.class public auto ansi Platform.NotImplementedException extends Platform.Object { }
.class public auto ansi Platform.NullReferenceException extends Platform.Object { }
.class public auto ansi Platform.OperationCancelledException extends Platform.Object { }
.class public auto ansi Platform.OutOfBoundsException extends Platform.Object { }
.class public auto ansi Platform.OutOfMemoryException extends Platform.Object { }
.class public auto ansi Platform.STAThreadAttribute extends Platform.Object { }
.class public auto ansi Platform.SizeT extends Platform.Object { }
.class public auto ansi Platform.TypeCode extends Platform.Object { }
