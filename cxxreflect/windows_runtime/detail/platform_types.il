
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This file defines a fake Platform assembly for use with C++ projects targeting the Windows Runtime.

.assembly Platform
{
    .ver 1:0:0:0
}

.module     Platform.winmd
.corflags   0x00000001 // ILONLY

.class  public auto ansi windowsruntime Platform.Object
{
    // TODO Platform.Object should probably expose the six IInspectable functions.
}

.class  public auto ansi windowsruntime Platform.Type
        extends class Platform.Object
        // TODO IMPLEMENTS
{
}

.class public auto ansi windowsruntime Platform.ValueType extends Platform.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

.class public auto ansi windowsruntime Platform.Attribute extends Platform.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

.class public auto ansi windowsruntime Platform.FlagsAttribute extends Platform.Object
{
    .method public hidebysig specialname rtspecialname instance
            void .ctor() runtime managed { }
}

// The value type fundamental types
.class public auto ansi windowsruntime Platform.Boolean        extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Char           extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.SByte          extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Byte           extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Int16          extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.UInt16         extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Int32          extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.UInt32         extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Int64          extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.UInt64         extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Single         extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Double         extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.IntPtr         extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.UIntPtr        extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Void           extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.TypedReference extends Platform.ValueType { }

.class public auto ansi windowsruntime Platform.AccessDeniedException       extends Platform.Object { }
.class public auto ansi windowsruntime Platform.Boolean                     extends Platform.Object { }
.class public auto ansi windowsruntime Platform.COMException                extends Platform.Object { }
.class public auto ansi windowsruntime Platform.CallbackContext             extends Platform.Object { }
.class public auto ansi windowsruntime Platform.ChangedStateException       extends Platform.Object { }
.class public auto ansi windowsruntime Platform.ClassNotRegisteredException extends Platform.Object { }
.class public auto ansi windowsruntime Platform.Delegate                    extends Platform.Object { }
.class public auto ansi windowsruntime Platform.DisconnectedException       extends Platform.Object { }
.class public auto ansi windowsruntime Platform.Enum                        extends Platform.ValueType { }
.class public auto ansi windowsruntime Platform.Exception                   extends Platform.Object { }
.class public auto ansi windowsruntime Platform.FailureException            extends Platform.Object { }
.class public auto ansi windowsruntime Platform.Guid                        extends Platform.Object { }
.class public auto ansi windowsruntime Platform.IBoxArray`1<T>              extends Platform.Object { }
.class public auto ansi windowsruntime Platform.IBox`1<T>                   extends Platform.Object { }
.class public auto ansi windowsruntime Platform.IDisposable                 extends Platform.Object { }
.class public auto ansi windowsruntime Platform.IValueType                  extends Platform.Object { }
.class public auto ansi windowsruntime Platform.InvalidArgumentException    extends Platform.Object { }
.class public auto ansi windowsruntime Platform.InvalidCastException        extends Platform.Object { }
.class public auto ansi windowsruntime Platform.MTAThreadAttribute          extends Platform.Object { }
.class public auto ansi windowsruntime Platform.MulticastDelegate           extends Platform.Object { }
.class public auto ansi windowsruntime Platform.NotImplementedException     extends Platform.Object { }
.class public auto ansi windowsruntime Platform.NullReferenceException      extends Platform.Object { }
.class public auto ansi windowsruntime Platform.OperationCancelledException extends Platform.Object { }
.class public auto ansi windowsruntime Platform.OutOfBoundsException        extends Platform.Object { }
.class public auto ansi windowsruntime Platform.OutOfMemoryException        extends Platform.Object { }
.class public auto ansi windowsruntime Platform.STAThreadAttribute          extends Platform.Object { }
.class public auto ansi windowsruntime Platform.SizeT                       extends Platform.Object { }
.class public auto ansi windowsruntime Platform.String                      extends Platform.Object { }
.class public auto ansi windowsruntime Platform.TypeCode                    extends Platform.Object { }
