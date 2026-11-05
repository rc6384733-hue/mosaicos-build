# Compila la ISO i386 de MosaicOS usando Docker Desktop en Windows.
$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $root

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Error 'Docker no está instalado o no está en PATH. Instala Docker Desktop.'
    exit 1
}

Write-Host '== Construyendo la imagen mosaicos-builder ==' -ForegroundColor Cyan
docker build -f desktop/docker/Dockerfile -t mosaicos-builder desktop
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ''
Write-Host '== Compilando MosaicOS 0.50 i386 ==' -ForegroundColor Cyan
docker run --rm -v "${root}:/work" mosaicos-builder i386
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ''
Write-Host 'ISO esperada en: desktop\dist\MosaicOS-0.50-i386.iso' -ForegroundColor Green
