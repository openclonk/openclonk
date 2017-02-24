pushd $env:BUILD_TARGET_FOLDER
trap {popd}

[void]([System.Reflection.Assembly]::LoadWithPartialName('Microsoft.Build'))
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
