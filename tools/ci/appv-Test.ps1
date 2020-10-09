pushd $env:BUILD_TARGET_FOLDER
trap {popd}

Add-Type -Path "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\Microsoft.Build.dll"
$projects = New-Object Microsoft.Build.Evaluation.ProjectCollection
$projects.SetGlobalProperty('Configuration', $env:CONFIGURATION)

Get-Item tests\*.vcxproj | %{
    $p = $projects.LoadProject($_.FullName)
    if ($p.GetPropertyValue('ConfigurationType') -eq 'Application') {
        $binary = $p.GetPropertyValue('TargetPath')
        if (Test-Path $binary) {
            & $binary "--gtest_output=xml:$binary.xml"
            $client = New-Object System.Net.WebClient
            $client.UploadFile("https://ci.appveyor.com/api/testresults/junit/$($env:APPVEYOR_JOB_ID)", "$binary.xml")
        }
    }
}
