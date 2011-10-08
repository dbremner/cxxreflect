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
