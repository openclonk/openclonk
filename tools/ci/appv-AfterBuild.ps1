pushd $env:BUILD_TARGET_FOLDER
trap {popd}

[void]([System.Reflection.Assembly]::LoadWithPartialName('Microsoft.Build'))
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
    function Push-AppveyorArtifact {
        param([string]$Path)
        "Uploading $Path.... (dry run)"
    }
}

Get-Item *.vcxproj | %{
    $p = $projects.LoadProject($_.FullName)
    if ($p.GetPropertyValue('ConfigurationType') -eq 'Application') {
        # For all executable files
        $binary = $p.GetPropertyValue('TargetPath')
        if (Test-Path $binary) {
            # Upload the executable itself as an artifact
            Push-AppveyorArtifact $binary
            $pdb = $p.ItemDefinitions['Link'].GetMetadataValue('ProgramDataBaseFile')
            if (Test-Path $pdb) {
                # If we generated a .pdb file, add source server information
                Add-SourceServerData $pdb
                Push-AppveyorArtifact $pdb
            }
        }
    }
}
