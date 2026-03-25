param (
    [string]$workingDir
)

# Define file names
$originalName = "enhanced_animations.script"
$renamedName = "enhanced_animations.script.mohidden"

# Get parent directory correctly
$parentDir = (Get-Item $workingDir).Parent.FullName
Write-Host Scanning dir: $parentDir

# Recursively get all files in parent directory
$files = Get-ChildItem -Path $parentDir -Recurse -File

# Look for the renamed version first
$renamedFiles = $files | Where-Object { $_.Name -eq $renamedName }

if ($renamedFiles.Count -gt 0) {
    foreach ($file in $renamedFiles) {
        $newNamePath = Join-Path -Path $file.DirectoryName -ChildPath $originalName
        Move-Item -Path $file.FullName -Destination $newNamePath -Force
        Write-Host "Renamed '$($file.FullName)' back to '$originalName'"
    }
} else {
    Write-Host "no '$renamedName' found in parent directory."
}