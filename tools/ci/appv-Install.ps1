$ErrorActionPreference='Stop'

if (-not $env:APPVEYOR) {
    function Set-AppveyorBuildVariable {
        param([string]$Name, [string]$Value)
        Set-Content -Path "env:$Name" -Value $Value
    }
}

# Calculate build paths.
$source_path = $env:APPVEYOR_BUILD_FOLDER
if ($source_path -eq $null) {
    $source_path = $PWD.Path
}
$build_path = $env:BUILD_TARGET_FOLDER
$slug = [System.IO.Path]::GetFileName($source_path)
$prefix = [System.IO.Path]::GetDirectoryName($source_path)
if ($build_path -eq $null) {
    $build_path = [System.IO.Path]::Combine($prefix, "${slug}-build")
    Set-AppveyorBuildVariable -Name BUILD_TARGET_FOLDER -Value $build_path
}
$deps_path = $env:BUILD_DEPS_FOLDER
if ($deps_path -eq $null) {
    $deps_path = [System.IO.Path]::Combine($prefix, "${slug}-deps")
    Set-AppveyorBuildVariable -Name BUILD_DEPS_FOLDER -Value $deps_path
}
$cache_path = $env:BUILD_CACHE_FOLDER
if ($cache_path -eq $null) {
    $cache_path = [System.IO.Path]::Combine($prefix, "${slug}-cache")
    Set-AppveyorBuildVariable -Name BUILD_CACHE_FOLDER -Value $cache_path
}

# Create folders we'll use
if (-not (Test-Path $build_path)) {
    [void](mkdir $build_path)
}
if (-not (Test-Path $deps_path)) {
    [void](mkdir $deps_path)
}
if (-not (Test-Path $cache_path)) {
    [void](mkdir $cache_path)
}

Write-Host "Source path: $source_path"
Write-Host "Building in: $build_path"
if ($build_path -eq $source_path) {
    Write-Host 'Build type: in-tree (deprecated)'
} else {
    Write-Host 'Build type: out-of-tree (recommended)'
}

function Update-BuildCache {
    param(
        [Parameter(Mandatory=$true)]
        [Uri]
        $Url,
        [string]
        $File = $Url.Segments[-1],
        [bool]
        $Force = $false
    )
    $local_file = $File
    if (-not [System.IO.Path]::IsPathRooted($local_file)) {
        $local_file = [System.IO.Path]::Combine($cache_path, $local_file)
    }
    Write-Host -NoNewline "Updating $([System.IO.Path]::GetFileName($local_file))... "
    $fi = New-Object System.IO.FileInfo $local_file
    [System.Net.HttpWebRequest]$req = [System.Net.WebRequest]::CreateHttp($Url)
    $req.Method = 'GET'
    # No need to do error handling because System.IO.FileInfo will return a
    # date in the past if the file does not exist
    $req.IfModifiedSince = $fi.LastWriteTime
    try {
        [System.Net.HttpWebResponse]$resp = $req.GetResponse()
        $target_stream = $fi.Create()
        $resp.GetResponseStream().CopyTo($target_stream)
        $target_stream.Dispose()
        $fi.LastWriteTime = $resp.LastModified
        Write-Host 'done'
    } catch [System.Net.WebException] {
        if ($_.Exception.Response.StatusCode -eq [System.Net.HttpStatusCode]::NotModified) {
            Write-Host 'unchanged'
            return
        }
        Write-Host 'failed!'
        throw $_.Exception
    }
}

# Update dependencies
pushd $deps_path
try {
    # If you want to use the build dependencies for yourself, please note that
    # this archive is a tarbomb, i.e. it doesn't contain a single root directory
    Update-BuildCache 'https://www.nosebud.de/~nh/openclonk/appveyor-deps.tar.xz'
    cmake -E tar xJ $cache_path\appveyor-deps.tar.xz
} finally {
    popd
}
