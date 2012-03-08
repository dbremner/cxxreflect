//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is one of a pair of test programs for CxxReflect:  AssemblyDumpCS and AssemblyDumpCPP.  The
// former uses the .NET Reflection APIs to write metadata for an assembly to a file; the latter uses
// the CxxReflect library to write metadata to a file in the same format.  The files can be diffed
// to validate that the CxxReflect library is functionally equivalent (where appropriate) to the
// .NET Reflection API. 

using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;

class Program
{
    const BindingFlags AllBindingFlags =
            BindingFlags.FlattenHierarchy |
            BindingFlags.Instance |
            BindingFlags.NonPublic |
            BindingFlags.Public |
            BindingFlags.Static;

    static void Dump(StringBuilder sb, Assembly a)
    {
        sb.AppendLine(String.Format("Assembly [{0}]", a.FullName));
        sb.AppendLine("!!BeginAssemblyReferences");
        foreach (AssemblyName x in a.GetReferencedAssemblies())
        {
            sb.AppendLine(String.Format(" -- AssemblyName [{0}]", x.FullName));
        }
        sb.AppendLine("!!EndAssemblyReferences");
        sb.AppendLine("!!BeginTypes");
        foreach (Type x in a.GetTypes().OrderBy(x => x.MetadataToken))
        {
            if (x.FullName == "System.__ComObject" || x.FullName == "System.Runtime.InteropServices.WindowsRuntime.DisposableRuntimeClass")
                continue;

            Dump(sb, x);
        }
        sb.AppendLine("!!EndTypes");
    }

    static void Dump(StringBuilder sb, Type t)
    {
        sb.AppendLine(String.Format(" -- Type [{0}] [${1:x8}]", t.FullName, t.MetadataToken));
        // TODO t.Assembly
        sb.AppendLine(String.Format("     -- AssemblyQualifiedName [{0}]", t.AssemblyQualifiedName));
        // TODO t.Attributes
        sb.AppendLine(String.Format("     -- BaseType [{0}]", t.BaseType != null ? t.BaseType.FullName : "NO BASE TYPE"));
        sb.AppendLine(String.Format("         -- AssemblyQualifiedName [{0}]", t.BaseType != null ? t.BaseType.AssemblyQualifiedName : "NO BASE TYPE"));
        // TODO t.BaseType
        // TODO t.ContainsGenericParameters
        // TODO t.DeclaringMethod
        // TODO t.DeclaringType
        // TODO t.FullName
        // TODO t.GenericParameterAttributes
        // TODO t.GenericParameterPosition
        // TODO t.GenericTypeArguments
        // TODO ...

        Func<bool, int> F = (x) => x ? 1 : 0;
        sb.AppendLine(String.Format("     -- IsTraits [{0}{1}{2}{3}{4}{5}{6}{7}] [{8}{9}{10}{11}{12}{13}{14}{15}] [{16}{17}{18}{19}{20}{21}{22}{23}] [{24}{25}{26}{27}{28}{29}{30}{31}] [{32}{33}{34}     ]",
            F(t.IsAbstract), F(t.IsAnsiClass), F(t.IsArray), F(t.IsAutoClass), F(t.IsAutoLayout), F(t.IsByRef), F(t.IsClass), F(t.IsCOMObject),
            F(t.IsContextful), F(t.IsEnum), F(t.IsExplicitLayout), F(t.IsGenericParameter), F(t.IsGenericType), F(t.IsGenericTypeDefinition), F(t.IsImport), F(t.IsInterface),
            F(t.IsLayoutSequential), F(t.IsMarshalByRef), F(t.IsNested), F(t.IsNestedAssembly), F(t.IsNestedFamANDAssem), F(t.IsNestedFamily), F(t.IsNestedFamORAssem), F(t.IsNestedPrivate),
            F(t.IsNestedPublic), F(t.IsNotPublic), F(t.IsPointer), F(t.IsPrimitive), F(t.IsPublic), F(t.IsSealed), F(t.IsSerializable), F(t.IsSpecialName),
            F(t.IsUnicodeClass), F(t.IsValueType), F(t.IsVisible)
            ));

        // TODO ...
        sb.AppendLine(String.Format("     -- Name [{0}]", t.Name));
        sb.AppendLine(String.Format("     -- Namespace [{0}]", t.Namespace));

        sb.AppendLine(String.Format("    !!BeginInterfaces"));
        foreach (Type it in t.GetInterfaces().OrderBy(x => x.MetadataToken))
        {
            sb.AppendLine(String.Format("     -- Interface [{0}] [${1:x8}]", it.FullName, it.MetadataToken));
        }
        sb.AppendLine(String.Format("    !!EndInterfaces"));

        sb.AppendLine(String.Format("    !!BeginCustomAttributes"));
        foreach (CustomAttributeData c in t.GetCustomAttributesData().OrderBy(x => x.Constructor.DeclaringType.MetadataToken).Where(x => x.Constructor.DeclaringType.FullName != "System.SerializableAttribute"))
        {
            sb.AppendLine(String.Format("     -- CustomAttribute [{0}]", c.Constructor.DeclaringType.FullName));
        }
        sb.AppendLine(String.Format("    !!EndCustomAttributes"));

        sb.AppendLine("    !!BeginConstructors");
        foreach (ConstructorInfo c in t.GetConstructors(AllBindingFlags).OrderBy(c => c.MetadataToken))
        {
            Dump(sb, c);
        }
        sb.AppendLine("    !!EndConstructors");

        sb.AppendLine("    !!BeginMethods");
        foreach (MethodInfo m in t.GetMethods(AllBindingFlags).OrderBy(m => m.MetadataToken))
        {
            Dump(sb, m);
        }
        sb.AppendLine("    !!EndMethods");

        /* TODO
        sb.AppendLine("    !!BeginFields");
        foreach (FieldInfo f in t.GetFields(AllBindingFlags).OrderBy(f => f.MetadataToken))
        {
            Dump(sb, f);
        }
        sb.AppendLine("    !!EndFields");
         */
    }

    static void Dump(StringBuilder sb, MethodBase m)
    {
        sb.AppendLine(String.Format("     -- Method [{0}] [${1:x8}]", m.Name, m.MetadataToken));
        sb.AppendLine("        !!BeginParameters");
        foreach (ParameterInfo p in m.GetParameters())
        {
            Dump(sb, p);
        }
        sb.AppendLine("        !!EndParameters");
    }

    static void Dump(StringBuilder sb, ParameterInfo p)
    {
        sb.AppendLine(String.Format("         -- [{0}] [${1:x8}] [{2}]", p.Name, p.MetadataToken, p.ParameterType.FullName));
    }

    static void Dump(StringBuilder sb, FieldInfo f)
    {
        sb.AppendLine(String.Format("     -- Field [{0}] [${1:x8}]", f.Name, f.MetadataToken));

        Func<bool, int> F = (x) => x ? 1 : 0;
        sb.AppendLine(String.Format("         -- Attributes [{0:x8}]", (int)f.Attributes));
        sb.AppendLine(String.Format("         -- Declaring Type [{0}]", f.DeclaringType.FullName));
        sb.AppendLine(String.Format("         -- IsTraits [{0}{1}{2}{3}{4}{5}{6}{7}] [{8}{9}{10}{11}    ]",
            F(f.IsAssembly), F(f.IsFamily), F(f.IsFamilyAndAssembly), F(f.IsFamilyOrAssembly), F(f.IsInitOnly), F(f.IsLiteral), F(f.IsNotSerialized), F(f.IsPinvokeImpl),
            F(f.IsPrivate), F(f.IsPublic), F(f.IsSpecialName), F(f.IsStatic)));
    }

    static void Main(string[] args)
    {
        String assemblyPath = "C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll";
        StringBuilder result = new StringBuilder();
        Dump(result, Assembly.ReflectionOnlyLoadFrom(assemblyPath));

        File.WriteAllText("c:\\jm\\mscorlib.cs.txt", result.ToString());
    }
}
