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

# Look for the original version
$originalFiles = $files | Where-Object { $_.Name -eq $originalName }
if ($originalFiles.Count -gt 0) {
    foreach ($file in $originalFiles) {
        $newNamePath = Join-Path -Path $file.DirectoryName -ChildPath $renamedName
        Move-Item -Path $file.FullName -Destination $newNamePath -Force
        Write-Host "Renamed '$($file.FullName)' to '$renamedName'"
    }
} else {
    Write-Host "no '$originalName' found in parent directory."
}
