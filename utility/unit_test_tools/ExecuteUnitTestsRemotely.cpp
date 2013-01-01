
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// CxxReflect Unit Test Driver Framework:  Remote Execution Task
//
// This is an MSBuild task that allows us to chain remote execution of ARM unit tests into a build.
// The task takes two arguments:  the path to the AppX package to be deployed on the remote machine
// and the path of the synchronization share being used to communicate with the remote host.
//
// The remote host must be running the unit_test_host.ps1 PowerShell daemon and must be configured
// with the same synchronization share that is used by the build on the client.  The (very simple)
// communication protocol is documented in the PowerShell script.  This task simply copies the
// required files to the synchronization share and waits to get the results back from the host,
// reporting success/failure status back to the build.

using ::cli::array;
using ::Microsoft::Build::Framework::RequiredAttribute;
using ::Microsoft::Build::Utilities::Task;
using ::System::Exception;
using ::System::String;
using ::System::StringComparison;
using ::System::Collections::Generic::List;
using ::System::IO::Directory;
using ::System::IO::File;
using ::System::IO::Path;
using ::System::Threading::Thread;

// This RAII class is used to ensure that any files created in the synchronization share are
// deleted before the task completes, regardless whether the task completes normally or abnormally.
private ref class CleanupShareOnExit
{
public:

    CleanupShareOnExit(::String^ path)
        : _path(path)
    {
    }

    ~CleanupShareOnExit()
    {
        // This is a best effort attempt to delete the files; if deletion fails, oh well...
        for each(::String^ file in ::Directory::GetFiles(_path))
        {
            try { ::File::Delete(file); } catch (...) { }
        }
    }

private:

    ::String^ _path;
};

public ref class ExecuteUnitTestsRemotely sealed : Task
{
public:

    [Required] property ::String^ Path;
    [Required] property ::String^ SynchronizationShare;

    virtual auto Execute() -> bool override
    {
        Log->LogMessage(L"================================================================================");
        Log->LogMessage(L"CxxReflect Unit Test Driver Framework:  Remote Execution Client");
        Log->LogMessage(L"================================================================================");

        // Ensure that when we return we correctly remove any files we create in the share.
        ::CleanupShareOnExit cleanup(SynchronizationShare);

        // We begin by finding the .appx and .cer files.  The 'Path' we are provided should be
        // the root directory in which the app package is built; the test package for deployment
        // should be located in a subdirectory whose name is suffixed by _Test.  This should be
        // the only subdirectory.  We search it for the .appx and .cer files, which we will then
        // deploy.
        Log->LogMessage(L"Searching for package to be deployed...");

        ::array<::String^>^ directories(::Directory::GetDirectories(Path));
        if (directories->Length != 1 || !directories[0]->EndsWith(L"_Test"))
            throw gcnew ::Exception(L"Test run failed:  could not locate test package directory");

        ::array<::String^>^ appxFiles(::Directory::GetFiles(directories[0], L"*.appx"));
        if (appxFiles->Length != 1)
            throw gcnew ::Exception(L"Test run failed:  could not locate .appx file for deployment");

        ::String^ appxPath(appxFiles[0]);
        ::String^ certPath(::Path::ChangeExtension(appxPath, ".cer"));

        if (!::File::Exists(certPath))
            throw gcnew ::Exception(L"Test run failed:  could not locate .cer file for deployment");

        Log->LogMessage(L"Package found:");
        Log->LogMessage(::String::Concat(L" * AppX:  ", appxPath));
        Log->LogMessage(::String::Concat(L" * Cert:  ", certPath));

        // A previous run may have failed prematurely, leaving leftover files in the share.  We
        // delete them here so they are not misinterpreted as up-to-date results.
        Log->LogMessage(L"Cleaning synchronization share for new test run...");

        for each(::String^ file in ::Directory::GetFiles(SynchronizationShare))
            ::File::Delete(file);

        // We now copy the .appx and .cer files to the synchronization share.  Note that they
        // must be copied before the start_job file in order to correctly synchronize with the
        // remote host.
        Log->LogMessage(L"Copying package to synchronization share...");

        ::String^ targetAppxPath(::String::Concat(SynchronizationShare, L"\\", ::Path::GetFileName(appxPath)));
        Log->LogMessage(::String::Concat(L" * ", appxPath, L" => ", targetAppxPath));
        ::File::Copy(appxPath, targetAppxPath);
            
        ::String^ targetCertPath(::String::Concat(SynchronizationShare, L"\\", ::Path::GetFileName(certPath)));
        Log->LogMessage(::String::Concat(L" * ", certPath, L" => ", targetCertPath));
        ::File::Copy(certPath, targetCertPath);

        // With the .appx and .cer files in place, we may command the remote host to start the
        // job by creating the start_job file in the synchronization share:
        Log->LogMessage(L"Commanding remote host to begin execution of test run...");

        ::File::CreateText(::String::Concat(SynchronizationShare, L"\\start_job"))->Close();

        // In order to more quickly diagnose errors when the remote host stops responding, it
        // will acknowledge the start_job command by writing a start_acknowledged file to the
        // synchronization share.  If this file is not written within a reasonable amount of
        // time, we abort the test run.
        Log->LogMessage(L"Waiting for acknowledgement from remote host...");

        unsigned waitCount(0);
        while (!::File::Exists(::String::Concat(SynchronizationShare, L"\\start_acknowledged")))
        {
            // This should complete within only a few iterations since the host polls at 1Hz, but we
            // wait longer just in case there is an unanticipated delay.  10 seconds won't delay the
            // build too much.
            ::Thread::Sleep(500);
            if (waitCount == 20)
            {
                Log->LogMessage(L"Remote host did not acknowledge command; aborting test run...");
                throw gcnew ::Exception(L"Test run failed: could not synchronize with remote machine");
            }
            ++waitCount;
        }

        // Wait for the run to complete on the remote host.  This may take a while.  We don't report
        // any interim status, so we only know when the job has completed.  We can't have a timeout
        // here because we don't know how long the tests will take to run.  So, it's up to the user
        // performing the build to wait an acceptable amount of time then investigate on the device.
        Log->LogMessage(L"Test run acknowledged by remote host.  Waiting for completion...");
        while (!::File::Exists(::String::Concat(SynchronizationShare, L"\\job_result")))
        {
            Log->LogMessage(L"Waiting...");
            ::Thread::Sleep(3000);
        }

        // Awwwww yeah, let's see if the tests passed:
        Log->LogMessage(L"Test run completed on remote; checking results...");
        ::List<::String^>^ results(gcnew ::List<::String^>(::File::ReadLines(::String::Concat(SynchronizationShare, L"\\job_result"))));
        if (results->Count < 1)
            throw gcnew ::Exception(L"Remote host returned malformed results file...");

        // If the run failed, write the test log to the MSBuild log so that we can investigate.
        if (results[0]->StartsWith(L"job aborted", StringComparison::OrdinalIgnoreCase))
        {
            Log->LogError(L"The test run failed. :'(  The test log is as follows:");
            for each (::String^ s in ::File::ReadLines(::String::Concat(SynchronizationShare, L"\\test_log.log")))
                Log->LogMessage(s);

            return false;
        }

        // If we succeeded, we can simply return; we're good to go:
        Log->LogMessage(L"The test run completed successfully!");
        return true;
    }
};
