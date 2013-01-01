# //                            Copyright James P. McNellis 2011 - 2013.                            //
# //                   Distributed under the Boost Software License, Version 1.0.                   //
# //     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

# CxxReflect Unit Test Driver Framework:  Remote Execution Host
#
# This is the remote execution host for remotely executing CxxReflect unit tests.  It runs on a
# Windows RT host and polls a synchronization share for work to be done.  A client, using the MSBuild
# tasks provided in the unit_test_client_tasks.dll assembly, commands this host to do work.  The flow
# looks roughly like so:
#
# 1.  The user starts the daemon on the Windows RT machine.
#
# 2.  The daemon polls the synchronization share at 1Hz, waiting for a file named 'start_job' to be
#     created in the share.
#
# 3.  The user initiates a remote test execution on the x86/x64 client by invoking one of the MSBuild
#     tasks.  This will first copy the .appx and .cer files to the synchronization share, then create
#     the'start_job' file.
#
# 4.  The daemon observes that the 'start_job' file has been created and breaks out of its polling
#     loop to service the request.  It deletes the 'start_job' file and writes a new file, named
#     'start_acknowledged' to the share.
#
#     The client waits for the 'start_acknowledged' file to be written.  When it detects the file, it
#     begins waiting for the unit tests to complete.  We use this 'start_acknowledged' file so that
#     the client can detect quickly if the host is not responding to the request.
#
# 5.  The daemon installs the certificate and executes the unit tests using the Visual Studio unit
#     test driver.  When the tests complete, it writes a job_result file to the synchronization share
#     with the status of the tests as the contents of the file (e.g. "job completed" or "job aborted").
#
#     In addition, the output from the Visual Studio unit test driver is tee'd to test_log.log in the
#     synchronization share, for the client to analyze.
#
# 6.  The daemon begins polling again, taking us back to step #2 (at least on the Windows RT side).
#
# 7.  In step 4, the client began polling for the unit tests to complete.  It does so by waiting for
#     the 'job_result' file to be written to the synchronization share.  When this file is written,
#     it reads the file to determine whether the job succeeded or not, and returns its status to
#     MSBuild, along with hopefully helpful diagnostic messages in the case of failure.
#
# Note that this daemon can be used without MSBuild:  one can simply copy and paste the .appx and
# .cer files into the synchronization share and then create the 'start_job' file (this is how the
# daemon was tested).
#
# Note also that this system is primitive and is in no way robust or scalable:  using the filesystem
# for synchronization is usually a poor solution, but it seems to be the best solution for this
# problem because we cannot write desktop apps for Windows RT (*sigh*).  There is nothing preventing
# two clients from both trying to write files to the synchronization share at the same time, and
# neither the daemon nor the MSBuild client makes substantial efforts to recover from unexpected
# errors.
#
# That said, this system is sufficient to run the CxxReflect unit tests on Windows RT during
# development, which is all we really need at the moment.  It works fine for a single client with a
# single PC and a single Windows RT device.

function poll($sync_share)
{
    Write-Host "I'm going to take a nap until work is available..." -NoNewLine

    do
    {
        Write-Host "z" -NoNewLine

        if (-not (Test-Path $sync_share\start_job))
        {
            Start-Sleep -s 1
            continue
        }

        ""
        "*yawn*"
        ""
        "Oh look, I have work to do!"

        "Let's clean up any leftover files from previous test jobs..."
        if (Test-Path $sync_share\job_result)
        {
            Remove-Item $sync_share\job_result
        }

        "Okay, all cleaned up.  Let's let the client know that we're starting the job..."
        Remove-Item $sync_share\start_job
        New-Item $sync_share\start_acknowledged -type file | out-null

        "Hmmmm, I wonder if we can find the .appx and .cer files that the client created..."
        $appx_files = Get-ChildItem -path $sync_share -filter "*.appx"
        $cer_files = Get-ChildItem -path $sync_share -filter "*.cer"

        if ($appx_files.Count -ne 1)
        {
            "Job aborted:  no appx file provided" | Tee-Object $sync_share\job_result
            continue
        }

        if ($cer_files.Count -ne 1)
        {
            "Job aborted:  no cer file provided" | Tee-Object $sync_share\job_result
            continue
        }

        $appx_file = $appx_files[0]
        $cer_file = $cer_files[0]

        if ($appx_file.BaseName -ne $cer_file.BaseName)
        {
            "Job aborted:  appx and cer files do not match" | Tee-Object $sync_share\job_result
            continue
        }

        "Yep, I found the files:"
        " * $appx_file"
        " * $cer_file"

        "First I'll install the certificate; let's hope we trust the client..."
        $cer_file_name = $cer_file.Name
        & certutil -addstore root $sync_share\$cer_file_name
        if ($LASTEXITCODE -ne 0)
        {
            "Job aborted:  certificate installation failed" | Tee-Object $sync_share\job_result
            continue
        }

        "Hmmm, where did we put the test executor; I know it's somewhere..."
        $executor_relative = "Microsoft Visual Studio 11.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe"
        $executor = "C:\Program Files (x86)\$executor_relative"
        if (-not (Test-Path $executor))
        {
            $executor = "C:\Program Files\$executor_relative"
        }

        if (-not (Test-Path $executor))
        {
            "job aborted:  failed to locate test executor" | Tee-Object $sync_share\job_result
            continue
        }

        "Ah, okay, I found it here:"
        " * $executor"

        "Execution time!"
        $appx_file_name = $appx_file.Name
        & $executor $sync_share\$appx_file_name /InIsolation | Tee-Object $sync_share\test_log.log
        if ($LASTEXITCODE -ne 0)
        {
            "Job aborted:  test execution failed" | Tee-Object $sync_share\job_result
            continue
        }

        ""
        "Job completed" | Tee-Object $sync_share\job_result

        ""
        Write-Host "Wow, that was hard work; time for another nap..." -NoNewLine
    }
    while (1)
}

"================================================================================"
"CxxReflect Unit Test Driver Framework:  Remote Execution Host"
"================================================================================"
""

poll \\aristotle\remote_sync