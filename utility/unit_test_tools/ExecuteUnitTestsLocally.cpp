
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// CxxReflect Unit Test Driver Framework:  Local Execution Task
//
// This is an MSBuild task that allows us to chain local execution of x86/x64 unit tests into a
// build.  The task takes three arguments:  the path to the DLL or AppX containing the tests to be
// executed, the kind of path that was provided ("DLL" or "AppX"), and the Platform for which the
// test binaries were compiled.
//
// This task simply executes the tests on the local host, using the Visual Studio test driver.

using ::cli::array;
using ::Microsoft::Build::Framework::RequiredAttribute;
using ::Microsoft::Build::Utilities::Task;
using ::System::Exception;
using ::System::String;
using ::System::StringComparison;
using ::System::Diagnostics::ProcessStartInfo;
using ::System::Diagnostics::Process;
using ::System::IO::Directory;
using ::System::IO::File;

public ref class ExecuteUnitTestsLocally sealed : Task
{
public:

    [Required] property ::String^ Path;
    [Required] property ::String^ Kind;
    [Required] property ::String^ Platform;

    virtual auto Execute() -> bool override
    {
        Log->LogMessage(L"================================================================================");
        Log->LogMessage(L"CxxReflect Unit Test Driver Framework:  Local Execution Client");
        Log->LogMessage(L"================================================================================");

        return ::String::Equals(Kind, L"DLL", ::StringComparison::OrdinalIgnoreCase)
            ? RunTests(Path)
            : RunTests(GetAppxPath(Path));
    }

private:

    // This runs the tests in the provided .dll or .appx file.  Note that if a .appx file is given,
    // the test driver does not attempt to install the associated certificate.  We could easily add
    // logic to install the required certificate.
    auto RunTests(String^ file) -> bool
    {
        Log->LogMessage(::String::Concat(L"Executing tests from ", file));

        ::ProcessStartInfo^ psi = gcnew ::ProcessStartInfo();
        psi->FileName = GetExecutorPath();
        psi->Arguments = ::String::Concat(L" /InIsolation /Platform:", CanonicalizePlatform(Platform), L" ", file);
        psi->UseShellExecute = false;

        ::Process^ utp(::Process::Start(psi));
        utp->WaitForExit();
        return utp->ExitCode == 0;
    }

    // This canonicalizes a platform name.  This is really only required to convert Win32 => x86.
    static auto CanonicalizePlatform(::String^ platform) -> ::String^
    {
        if (platform == L"Win32") { return L"x86"; }
        if (platform == L"x64")   { return L"x64"; }
        if (platform == L"ARM")   { return L"ARM"; }
        throw gcnew ::Exception(L"An invalid platform was provided");
    }

    // This gets the path to the AppX file given the root "app packages" directory for a particular
    // platform and configuration build.  In the MSBuild script, we can't compute the path to the
    // .appx file without doing a lot of extra work.  Since we know roughly where the .appx file is
    // relative to the root "app packages" directory, we simply compute it here.
    static auto GetAppxPath(::String^ root) -> ::String^
    {
        ::array<::String^>^ directories(::Directory::GetDirectories(root));
        if (directories->Length != 1 || !directories[0]->EndsWith(L"_Test"))
            throw gcnew ::Exception(L"Test run failed:  could not locate test package directory");

        ::array<::String^>^ appxFiles(::Directory::GetFiles(directories[0], L"*.appx"));
        if (appxFiles->Length != 1)
            throw gcnew ::Exception(L"Test run failed:  could not locate .appx file for deployment");

        return appxFiles[0];
    }

    // This gets the path to the Visual Studio unit test command line executor.  It assumes that
    // Visual Studio is installed in the default location either on x86 or x64.
    static auto GetExecutorPath() -> ::String^
    {
        ::String^ fragment(gcnew ::String(
            "Microsoft Visual Studio 11.0\\Common7\\IDE\\CommonExtensions\\Microsoft\\TestWindow\\vstest.console.exe"));

        ::String^ path0(::String::Concat(gcnew ::String("C:\\Program Files (x86)\\"), fragment));
        if (::File::Exists(path0))
            return path0;

        ::String^ path1(::String::Concat(gcnew ::String("C:\\Program Files\\"), fragment));
        if (::File::Exists(path1))
            return path1;

        throw gcnew ::Exception("Oh snap!  We couldn't find the test executor!");
    }
};
