$ErrorActionPreference='Stop'

switch ($env:PLATFORM) {
    'Win32' {
        $cmake_generator = 'Visual Studio 14 2015'
        $qt = 'C:\Qt\5.7\msvc2015'
    }
    'x64' {
        $cmake_generator = 'Visual Studio 14 2015 Win64'
        $qt = 'C:\Qt\5.7\msvc2015_64'
    }
}

pushd $env:BUILD_TARGET_FOLDER
try {
    $(cmake --version)[0]
    $ErrorActionPreference='SilentlyContinue'
    cmake -G $cmake_generator -DCMAKE_PREFIX_PATH:PATH="$env:BUILD_DEPS_FOLDER\$env:PLATFORM;$qt_path" $env:APPVEYOR_BUILD_FOLDER 2>&1
    $ErrorActionPreference='Stop'
    if ($LASTEXITCODE -ne 0) {
        if (Test-Path CMakeFiles\CMakeOutput.log) {
            Push-AppveyorArtifact CMakeFiles\CMakeOutput.log
        }
        if (Test-Path CMakeFiles\CMakeError.log) {
            Push-AppveyorArtifact CMakeFiles\CMakeError.log
        }
        throw "CMake invocation failed with code $LASTEXITCODE"
    }
} finally {
    popd
}
