param(
    [Parameter(Mandatory = $true)]
    [string]$RootDir,
    [string]$FfmpegDir = "C:\bin\ffmpeg-8.1.2-full_build\bin"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$root = [IO.Path]::GetFullPath($RootDir).TrimEnd('\', '/')
$ffmpeg = Join-Path $FfmpegDir "ffmpeg.exe"
$ffprobe = Join-Path $FfmpegDir "ffprobe.exe"

if (-not (Test-Path -LiteralPath $root -PathType Container)) {
    throw "Root directory does not exist: $root"
}
if (-not (Test-Path -LiteralPath $ffmpeg -PathType Leaf) -or
    -not (Test-Path -LiteralPath $ffprobe -PathType Leaf)) {
    throw "ffmpeg and ffprobe were not found in: $FfmpegDir"
}

$files = @(Get-ChildItem -LiteralPath $root -Recurse -File -Filter "*.wav" |
    Sort-Object FullName)
$beforeBytes = ($files | Measure-Object Length -Sum).Sum
$converted = 0
$skipped = 0

foreach ($file in $files) {
    $path = $file.FullName
    $relative = $path.Substring($root.Length + 1)
    $originalTimestamp = $file.LastWriteTimeUtc
    $codec = & $ffprobe -v error -select_streams a:0 `
        -show_entries stream=codec_name `
        -of default=noprint_wrappers=1:nokey=1 -- $path
    if ($LASTEXITCODE -ne 0) {
        throw "Could not inspect WAV codec: $relative"
    }
    if (($codec | Select-Object -First 1) -eq "adpcm_ima_wav") {
        ++$skipped
        continue
    }

    $temporary = Join-Path $file.DirectoryName `
        (".{0}.ima-{1}.wav" -f $file.Name, [guid]::NewGuid().ToString("N"))
    $backup = Join-Path $file.DirectoryName `
        (".{0}.backup-{1}" -f $file.Name, [guid]::NewGuid().ToString("N"))
    try {
        & $ffmpeg -n -hide_banner -loglevel error -i $path -map_metadata -1 `
            -c:a adpcm_ima_wav $temporary
        if ($LASTEXITCODE -ne 0) {
            throw "ffmpeg encode failed for: $relative"
        }

        $encodedCodec = & $ffprobe -v error -select_streams a:0 `
            -show_entries stream=codec_name `
            -of default=noprint_wrappers=1:nokey=1 -- $temporary
        if ($LASTEXITCODE -ne 0 -or
            ($encodedCodec | Select-Object -First 1) -ne "adpcm_ima_wav") {
            throw "Encoded codec validation failed for: $relative"
        }

        & $ffmpeg -v error -i $temporary -f null NUL
        if ($LASTEXITCODE -ne 0) {
            throw "Full decode validation failed for: $relative"
        }

        [IO.File]::Replace($temporary, $path, $backup)
        [IO.File]::SetLastWriteTimeUtc($path, $originalTimestamp)
        Remove-Item -LiteralPath $backup -Force
        ++$converted
        if (($converted % 25) -eq 0) {
            Write-Host "Converted $converted of $($files.Count) WAV files"
        }
    }
    finally {
        if (Test-Path -LiteralPath $temporary -PathType Leaf) {
            Remove-Item -LiteralPath $temporary -Force
        }
    }
}

$afterFiles = @(Get-ChildItem -LiteralPath $root -Recurse -File -Filter "*.wav")
$afterBytes = ($afterFiles | Measure-Object Length -Sum).Sum
$ratio = if ($afterBytes) { $beforeBytes / $afterBytes } else { 0 }
$reduction = if ($beforeBytes) { (1 - $afterBytes / $beforeBytes) * 100 } else { 0 }

Write-Host "Converted: $converted"
Write-Host "Already IMA ADPCM: $skipped"
Write-Host ("Size: {0:N2} MiB -> {1:N2} MiB ({2:N2}x, {3:N2}% smaller)" -f `
    ($beforeBytes / 1MB), ($afterBytes / 1MB), $ratio, $reduction)
