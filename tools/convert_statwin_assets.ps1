param(
    [string]$SourceDir = "C:\Magic\Program\Statwin",
    [string]$OutputDir = "C:\Magic\Program\Statwin_SDL",
    [string]$FfmpegDir = "C:\bin\ffmpeg-8.1.2-full_build\bin"
)

$ErrorActionPreference = "Stop"

$ffmpeg = Join-Path $FfmpegDir "ffmpeg.exe"
$ffprobe = Join-Path $FfmpegDir "ffprobe.exe"
$source = [IO.Path]::GetFullPath($SourceDir).TrimEnd('\', '/')
$output = [IO.Path]::GetFullPath($OutputDir).TrimEnd('\', '/')

if ($source -eq $output -or $output.StartsWith($source + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputDir must not be the source directory or one of its children."
}
if (-not (Test-Path -LiteralPath $source -PathType Container)) {
    throw "Source directory does not exist: $source"
}
if (-not (Test-Path -LiteralPath $ffmpeg -PathType Leaf) -or
    -not (Test-Path -LiteralPath $ffprobe -PathType Leaf)) {
    throw "ffmpeg and ffprobe were not found in: $FfmpegDir"
}

New-Item -ItemType Directory -Force -Path $output | Out-Null

function Invoke-Ffmpeg {
    param([string[]]$Arguments)
    & $ffmpeg -y -hide_banner -loglevel error @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "ffmpeg failed with exit code $LASTEXITCODE"
    }
}

function Uses-GreenColorKey {
    param([string]$Name)
    return $Name -match '(?i)-(sprt|mask)\.bmp$'
}

function Uses-BlackColorKey {
    param([string]$Name)
    return $Name -match '(?i)skulls\.bmp$' -or $Name -match '(?i)^S-.*wiz[ry]\.bmp$'
}

function AviUses-BlackColorKey {
    param([string]$Name)
    if ($Name -match '(?i)-hit\.avi$') {
        return $true
    }
    return $Name -match '(?i)^(B|W|U|G|R|M|Dg)_.+\.avi$'
}

foreach ($file in Get-ChildItem -LiteralPath $source -File -Filter "*.bmp" | Sort-Object Name) {
    $destination = Join-Path $output ($file.BaseName + ".png")
    if (Uses-GreenColorKey $file.Name) {
        Invoke-Ffmpeg @("-i", $file.FullName, "-vf", "colorkey=0x00ff00:0.001:0.0,format=rgba", "-frames:v", "1", $destination)
    }
    elseif (Uses-BlackColorKey $file.Name) {
        Invoke-Ffmpeg @("-i", $file.FullName, "-vf", "colorkey=0x000000:0.001:0.0,format=rgba", "-frames:v", "1", $destination)
    }
    else {
        Invoke-Ffmpeg @("-i", $file.FullName, "-frames:v", "1", $destination)
    }
    Write-Host "PNG  $($file.Name)"
}

$aviFiles = @(Get-ChildItem -LiteralPath $source -File -Filter "*.avi" | Sort-Object Name)
$mtgend = Join-Path (Split-Path $source -Parent) "Mtgend.avi"
if (Test-Path -LiteralPath $mtgend -PathType Leaf) {
    $aviFiles += Get-Item -LiteralPath $mtgend
}

foreach ($file in $aviFiles) {
    $webp = Join-Path $output ($file.BaseName + ".webp")
    if (AviUses-BlackColorKey $file.Name) {
        Invoke-Ffmpeg @(
            "-i", $file.FullName,
            "-vf", "colorkey=0x000000:0.01:0.0,format=bgra",
            "-c:v", "libwebp_anim", "-lossless", "0", "-quality", "90", "-compression_level", "4",
            "-loop", "1", "-an", $webp
        )
    }
    else {
        Invoke-Ffmpeg @(
            "-i", $file.FullName,
            "-vf", "format=bgra",
            "-c:v", "libwebp_anim", "-lossless", "0", "-quality", "90", "-compression_level", "4",
            "-loop", "1", "-an", $webp
        )
    }
    Write-Host "WEBP $($file.Name)"

    $audioStreams = & $ffprobe -v error -select_streams a:0 -show_entries stream=index -of csv=p=0 $file.FullName
    if ($LASTEXITCODE -ne 0) {
        throw "ffprobe failed for $($file.FullName)"
    }
    if ($audioStreams) {
        $wav = Join-Path $output ($file.BaseName + ".wav")
        Invoke-Ffmpeg @("-i", $file.FullName, "-map", "0:a:0", "-c:a", "pcm_s16le", $wav)
        Write-Host "WAV  $($file.Name)"
    }
}

Write-Host "Converted assets written to $output"
