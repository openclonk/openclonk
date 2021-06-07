pushd $env:BUILD_TARGET_FOLDER
trap {popd}

$ErrorActionPreference = 'Stop'

cmd /D /Q /C "`"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\vsdevcmd.bat`" -arch=${env:PLATFORM} > NUL && SET" | %{
    $k, $v = $_.Split('=', 2)
    [System.Environment]::SetEnvironmentVariable($k, $v)
}

Add-Type -Path "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\Microsoft.Build.dll"
$projects = New-Object Microsoft.Build.Evaluation.ProjectCollection
$projects.SetGlobalProperty('Configuration', $env:CONFIGURATION)

function Resolve-CanonicalPath {
    param([string]$Path)
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

public static class ResolveCanonicalPath {
    [DllImport("kernel32.dll", EntryPoint="CreateFileW", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern SafeFileHandle CreateFile(
        string lpFileName,
        int dwDesiredAccess,
        int dwShareMode,
        IntPtr securityAttributes,
        int dwCreationDisposition,
        int dwFlagsAndAttributes,
        IntPtr hTemplateFile);
    [DllImport("kernel32.dll", EntryPoint="GetFinalPathNameByHandleW", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern int GetFinalPathNameByHandle(
        IntPtr hFile,
        [In, Out] System.Text.StringBuilder lpszFilePath,
        int cchFilePath,
        int dwFlags);
    }
'@
    # Ask the file system to provide us with the canonical path, which includes
    # proper capitalization
    $fh = [ResolveCanonicalPath]::CreateFile($Path, 0, 2, [System.IntPtr]::Zero, 3, 0x2000000, [System.IntPtr]::Zero)
    try {
        $real_path = New-Object System.Text.StringBuilder 512
        $rp_len = [ResolveCanonicalPath]::GetFinalPathNameByHandle($fh.DangerousGetHandle(), $real_path, $real_path.Capacity, 0)
        if ($rp_len -lt 0) {
            throw [System.ComponentModel.Win32Exception]::new([System.Runtime.InteropServices.Marshal]::GetLastWin32Error())
        }
        $real_path = $real_path.ToString()
        if ($real_path.StartsWith('\\?\')) {
            return $real_path.Substring(4)
        }
        return $real_path
    } finally {
        $fh.Close()
    }
}

function Add-SourceServerData {
    param([string]$pdb)
    
    $srctool = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\srcsrv\srctool.exe"
    $pdbstr = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\srcsrv\pdbstr.exe"
    
    $temp_name = New-TemporaryFile
    try {
        $temp = New-Object System.IO.StreamWriter $temp_name.OpenWrite()
        $temp.WriteLine(@"
SRCSRV: ini ------------------------------------------------
VERSION=2
VERCTRL=http
SRCSRV: variables ------------------------------------------
DEPOT=https://raw.githubusercontent.com/$env:APPVEYOR_REPO_NAME/%COMMIT%/
COMMIT=$env:APPVEYOR_REPO_COMMIT
SRCSRVTRG=%DEPOT%%var2%
SRCSRV: source files ---------------------------------------
"@)
        & $srctool -r $pdb | %{
            Resolve-CanonicalPath $_
        } | ?{
            # Filter everything outside of the source path
            $_.StartsWith($env:APPVEYOR_BUILD_FOLDER, [System.StringComparison]::OrdinalIgnoreCase)
        } | ?{
            # Filter everything that doesn't exist
            Test-Path $_
        } | %{
            # Strip source folder prefix for building an URL
            $path = $_.Substring($env:APPVEYOR_BUILD_FOLDER.Length)
            $path = $path.TrimStart(@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
            $path = $path.Replace('\','/')
            $temp.WriteLine("$_*$path")
        }
        $temp.WriteLine('SRCSRV: end ------------------------------------------------')
        $temp.Close()
        # Write source info to PDB
        & $pdbstr -w -p:$pdb -i:$($temp_name.FullName) -s:srcsrv
    } finally {
        $temp_name.Delete()
    }
}

if (-not $env:APPVEYOR) {
    function appveyor {
        param ([string]$action, [string[]]$rest)
    }
}

# Add source server information to all debug symbols and push the
# binaries/symbols to AppVeyor
Get-Item *.vcxproj | %{
    $p = $projects.LoadProject($_.FullName)
    if ($p.GetPropertyValue('ConfigurationType') -eq 'Application') {
        # For all executable files
        $binary = $p.GetPropertyValue('TargetPath')
        if (Test-Path $binary) {
            # Upload the executable itself as an artifact
            appveyor PushArtifact $binary
            $pdb = $p.ItemDefinitions['Link'].GetMetadataValue('ProgramDataBaseFile')
            if (Test-Path $pdb) {
                # If we generated a .pdb file, add source server information
                Add-SourceServerData $pdb
                appveyor PushArtifact $pdb
            }
        }
    }
}

if ($env:DEPLOYMENT_URL -and $env:DEPLOYMENT_SECRET) {
    # Build a zip file with the main executable and its dependencies
    $deploy_dir = "${env:BUILD_TARGET_FOLDER}\deployment"
    [void](mkdir $deploy_dir)

    $openclonk = $projects.LoadedProjects | ?{ $_.GetPropertyValue('ProjectName') -eq 'openclonk' }
    $c4group = $projects.LoadedProjects | ?{ $_.GetPropertyValue('ProjectName') -eq 'c4group' }
    $groups = $projects.LoadedProjects | ?{ $_.GetPropertyValue('ProjectName') -eq 'groups' }
    if ($openclonk -and $c4group -and $groups) {
        $groupFiles = ($groups.GetItems('CustomBuild') |
            ?{ $_.GetMetadataValue('AdditionalInputs') -split ';' -contains $c4group.GetPropertyValue('TargetPath') }).
            GetMetadataValue('Outputs')
    }

    # Copy executables, Qt
    C:\Qt\5.11\msvc2017_64\bin\windeployqt.exe --no-translations --no-compiler-runtime --dir $deploy_dir $openclonk.GetPropertyValue('TargetPath')
    Copy-Item $openclonk.GetPropertyValue('TargetPath') "${deploy_dir}\"
    Copy-Item $c4group.GetPropertyValue('TargetPath') "${deploy_dir}\"

    # Copy other DLLs openclonk depends on
    function Get-Imports {
        param([string]$file)
        (dumpbin /imports $file | ?{ $_ -match ' {4}[\w-]+\.dll'}).Trim()
    }
    $unresolved = [System.Collections.Queue]::new()
    Get-Imports $openclonk.GetPropertyValue('TargetPath') | %{ $unresolved.Enqueue($_) }
    Get-Imports $c4group.GetPropertyValue('TargetPath') | %{ $unresolved.Enqueue($_) }
    while ($unresolved.Count -gt 0) {
        $library = $unresolved.Dequeue()
        $file = Join-Path $deploy_dir $library
        $dep_file = "${env:BUILD_DEPS_FOLDER}\${env:PLATFORM}\bin\${library}"
        if (-not (Test-Path $file) -and (Test-Path $dep_file)) {
            Write-Host "Bundling ${library}"
            Copy-Item $dep_file $file
            Get-Imports $file | %{ $unresolved.Enqueue($_) }
        }
    }

    # Copy generated group files
    $groupFiles | %{ Copy-Item $_ $deploy_dir }

    # Create archive
    $archive_name = "OpenClonk-win-${env:PLATFORM}.zip"
    7z a -mx=9 -y -r -- $archive_name "${deploy_dir}\*"

    $deployment_user, $deployment_password = $env:DEPLOYMENT_SECRET.Split(':', 2)
    $timestamp = Get-Date -Date (Get-Date $env:APPVEYOR_REPO_COMMIT_TIMESTAMP).ToUniversalTime() -UFormat '%Y-%m-%dT%H:%M:%SZ'
    $deployment_url = [System.UriBuilder]::new("${env:DEPLOYMENT_URL}${timestamp}-${env:APPVEYOR_REPO_BRANCH}-$($env:APPVEYOR_REPO_COMMIT.Substring(0, 9))/${archive_name}")
    $deployment_url.UserName = $deployment_user
    $deployment_url.Password = $deployment_password
    $deployment_auth = [System.Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes("${deployment_user}:${deployment_password}"))

    [System.Net.ServicePointManager]::SecurityProtocol = 'Tls12'
    [System.Net.HttpWebRequest]$req = [System.Net.WebRequest]::CreateHttp($deployment_url.Uri)
    $deployment_url.Password = '********'
    $req.Timeout = 30 * 60 * 1000
    $req.Method = 'POST'
    $req.ContentType = 'application/octet-stream'
    $req.AllowWriteStreamBuffering = $false
    $req.SendChunked = $true
    $req.Headers.Add('Authorization', "Basic ${deployment_auth}")
    $fileStream = [System.IO.File]::OpenRead((Resolve-Path $archive_name))
    Write-Host "Uploading to $($deployment_url.Uri)"
    $reqStream = $req.GetRequestStream()
    $fileStream.CopyTo($reqStream)
    $fileStream.Close()
    $reqStream.Close()
    try {
        [System.Net.HttpWebResponse]$resp = $req.GetResponse()
        Write-Host "Upload successful: $([uint32]$resp.StatusCode) $($resp.StatusDescription)"
    } catch [System.Net.WebException] {
        Write-Host "Upload failed: $($_.Exception.Status) $($_.Exception.Response.StatusDescription)"
    }
}
