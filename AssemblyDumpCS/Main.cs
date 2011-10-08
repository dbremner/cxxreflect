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
    static void Dump(StringBuilder sb, Assembly a)
    {
        sb.AppendLine(String.Format("Assembly [{0}]", a.FullName));
        sb.AppendLine("!!BeginTypes");
        foreach (Type t in a.GetTypes().OrderBy(x => x.MetadataToken))
        {
            Dump(sb, t);
        }
        sb.AppendLine("!!EndTypes");
    }

    static void Dump(StringBuilder sb, Type t)
    {
        sb.AppendLine(String.Format(" -- Type [{0}] [${1}]", t.FullName, t.MetadataToken));
    }

    static void Main(string[] args)
    {
        String assemblyPath = "C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll";
        StringBuilder result = new StringBuilder();
        Dump(result, Assembly.ReflectionOnlyLoadFrom(assemblyPath));

        File.WriteAllText("d:\\jm\\devmscorlib.cs.txt", result.ToString());
    }
}
