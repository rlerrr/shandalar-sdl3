param(
    [string]$SourceDir = "C:\Magic\Program - Copy",
    [string]$OutputDir = "C:\dev\shandalar-sdl3\out\bin\shandalar-wasm",
    [string]$EmscriptenRoot = "",
    [string]$Python = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$source = [IO.Path]::GetFullPath($SourceDir).TrimEnd('\', '/')
$output = [IO.Path]::GetFullPath($OutputDir).TrimEnd('\', '/')

if (-not (Test-Path -LiteralPath $source -PathType Container)) {
    throw "Source directory does not exist: $source"
}

if (-not $EmscriptenRoot) {
    if ($env:EMSDK) {
        $EmscriptenRoot = Join-Path $env:EMSDK "upstream\emscripten"
    }
    else {
        $EmscriptenRoot = "C:\bin\emsdk\upstream\emscripten"
    }
}
$emscripten = [IO.Path]::GetFullPath($EmscriptenRoot)
$packager = Join-Path $emscripten "tools\file_packager.py"
if (-not (Test-Path -LiteralPath $packager -PathType Leaf)) {
    throw "Emscripten file_packager.py was not found: $packager"
}

if (-not $Python) {
    if ($env:EMSDK_PYTHON -and (Test-Path -LiteralPath $env:EMSDK_PYTHON -PathType Leaf)) {
        $Python = $env:EMSDK_PYTHON
    }
    else {
        $emsdkRoot = Split-Path (Split-Path $emscripten -Parent) -Parent
        $bundledPython = Get-ChildItem -LiteralPath (Join-Path $emsdkRoot "python") `
            -Filter python.exe -File -Recurse -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($bundledPython) {
            $Python = $bundledPython.FullName
        }
        else {
            $pythonCommand = Get-Command python -ErrorAction SilentlyContinue
            if (-not $pythonCommand) {
                throw "Python was not found. Pass -Python or set EMSDK_PYTHON."
            }
            $Python = $pythonCommand.Source
        }
    }
}
if (-not (Test-Path -LiteralPath $Python -PathType Leaf)) {
    throw "Python executable does not exist: $Python"
}

New-Item -ItemType Directory -Force -Path $output | Out-Null

$dataFile = Join-Path $output "shandalar-assets.data"
$loaderFile = Join-Path $output "shandalar-assets.js"
$excludePatterns = @(
    "*.cat",
    "*.webp",
    "*.avi",
    "*.hlp",
    "*.wav",
    # file_packager processes exclusions in order; later ! patterns re-include
    # the compressed media needed by the SDL3 Shandalar build.
    "!*/statscrn.wav",
    "!*/sound/*.wav",
    "!*/duelsounds/*.wav",
    "*/duelsounds/exp1_*.wav",
    "*/duelsounds/shell_*.wav",
    "!*/statwin_sdl/cointoss_heads.webp",
    "!*/statwin_sdl/cointoss_heads.wav",
    "!*/statwin_sdl/cointoss_tails.webp",
    "!*/statwin_sdl/cointoss_tails.wav",
    "*/exp1art/*",
    "*/facemaker/*",
    "*/manalink/*",
    "*/playdeck/*",
    "*/playface/*",
    "*/savegame/*",
    "*/screennames/*",
    "*/shellart/*",
    "*/spr/*",
    "*/spr800/*",
    "*/statwin/*"
)

$arguments = @(
    $packager,
    $dataFile,
    "--preload",
    "$source@/assets",
    "--exclude"
) + $excludePatterns + @(
    "--js-output=$loaderFile",
    "--export-es6",
    "--use-preload-cache",
    "--indexedDB-name=SHANDALAR_ASSET_BUNDLE"
)

Write-Host "Building Shandalar asset bundle from:"
Write-Host "  $source"
Write-Host "Writing:"
Write-Host "  $dataFile"
Write-Host "  $loaderFile"

& $Python @arguments
if ($LASTEXITCODE -ne 0) {
    throw "file_packager.py failed with exit code $LASTEXITCODE"
}

$data = Get-Item -LiteralPath $dataFile
$loader = Get-Item -LiteralPath $loaderFile
Write-Host ("Created {0} ({1:N2} MiB)" -f $data.Name, ($data.Length / 1MB))
Write-Host ("Created {0} ({1:N2} KiB)" -f $loader.Name, ($loader.Length / 1KB))
