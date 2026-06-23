# build.ps1 — Script de compilação do Desktop Rain
# Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
#
# USO:
#   .\build.ps1              # build debug (padrão)
#   .\build.ps1 debug        # build debug explícito
#   .\build.ps1 release      # build release otimizado
#
# PRÉ-REQUISITOS:
#   - MinGW-w64 (g++, windres) disponível no PATH
#   - Executar no diretório raiz do projeto (onde está o src/)
#
# SAÍDA:
#   build\debug\Desktop Rain.exe   (modo debug)
#   build\release\Desktop Rain.exe (modo release)

param(
    [string]$Mode = "debug"
)

$Mode = $Mode.ToLower()
$OutputDir = "build\$Mode"

if (-not (Test-Path $OutputDir)) {
    Write-Host "Creating directory: $OutputDir"
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

$Sources  = @("src\*.cpp", "src\modes\*.cpp")
$Output   = "$OutputDir\Desktop Rain.exe"
$Libs     = "-lgdi32 -ld2d1 -ldwrite -lole32 -luuid -lcomdlg32 -lshell32 -lwinmm -ldwmapi"
$ResObj   = $null

if (Test-Path "resources.rc") {
    Write-Host "Compiling resources..."
    windres resources.rc -o resources.o
    $ResObj = "resources.o"
}

if ($Mode -eq "release") {
    $Flags = "-O3 -std=c++23 -mwindows -municode -s -DNDEBUG -DUNICODE -D_UNICODE"
    Write-Host "Building Release..."
} else {
    $Flags = "-g -std=c++23 -Wall -mwindows -municode -DUNICODE -D_UNICODE"
    Write-Host "Building Debug..."
}

$SourceArgs = ($Sources | ForEach-Object { $_ }) -join " "
$ResArg     = if ($ResObj) { $ResObj } else { "" }
$Cmd        = "g++ $SourceArgs $ResArg -o `"$Output`" $Flags $Libs"

Write-Host $Cmd
Invoke-Expression $Cmd
$BuildStatus = $LASTEXITCODE

if ($ResObj -and (Test-Path "resources.o")) {
    Remove-Item "resources.o"
}

if ($BuildStatus -eq 0) {
    Write-Host "Build successful: $Output"
} else {
    Write-Host "Build failed!"
    exit 1
}
