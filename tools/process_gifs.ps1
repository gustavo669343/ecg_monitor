param(
    [string]$InputDir = "gifs",
    [int]$DefaultScale = 6
)

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host "   GIF Converter & Registry Generator for ESP32-S3    " -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan

# 1. Tìm các file .gif
$files = @()
if (Test-Path $InputDir) {
    $files = Get-ChildItem -Path $InputDir -Filter "*.gif"
}

# Nếu thư mục rỗng, tìm các file .gif ở thư mục gốc
if ($files.Count -eq 0) {
    $files = Get-ChildItem -Filter "*.gif"
}

if ($files.Count -eq 0) {
    Write-Host "[CANH BAO] Khong tim thay file .gif nao trong '$InputDir' hoac thu muc goc!" -ForegroundColor Yellow
    Write-Host "Hay chep cac file .gif vao thu muc 'gifs/' roi chay lai script!"
    exit 0
}

Write-Host "Tim thay $($files.Count) file .gif:" -ForegroundColor Green
foreach ($f in $files) {
    Write-Host " -> $($f.Name) ($($f.Length) bytes)"
}

$registryEntries = @()

foreach ($file in $files) {
    $cleanName = ($file.BaseName -replace '[^a-zA-Z0-9]', '_').ToLower().Trim('_')
    $headerName = "$($cleanName.Substring(0,1).ToUpper())$($cleanName.Substring(1))Gif.h"
    $outputPath = Join-Path "include" $headerName
    $varName = "${cleanName}_gif"

    Write-Host "`nDang convert: $($file.Name) -> $outputPath..." -ForegroundColor Yellow

    $bytes = [System.IO.File]::ReadAllBytes($file.FullName)

    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine("#pragma once")
    [void]$sb.AppendLine("#include <Arduino.h>")
    [void]$sb.AppendLine()
    [void]$sb.AppendLine("const uint8_t ${varName}[] PROGMEM = {")

    for ($i = 0; $i -lt $bytes.Length; $i++) {
        if (($i % 16) -eq 0) {
            if ($i -gt 0) { [void]$sb.AppendLine() }
            [void]$sb.Append("    ")
        }
        [void]$sb.Append(("0x{0:x2}, " -f $bytes[$i]))
    }

    [void]$sb.AppendLine()
    [void]$sb.AppendLine("};")
    [void]$sb.AppendLine("const size_t ${varName}_size = $($bytes.Length);")

    [System.IO.File]::WriteAllText($outputPath, $sb.ToString())

    $registryEntries += [PSCustomObject]@{
        Name = $cleanName
        VarName = $varName
        Header = $headerName
        Scale = $DefaultScale
    }
}

# Tự động cập nhật include/GifRegistry.h
$regPath = "include/GifRegistry.h"
Write-Host "`nDang cap nhat $regPath..." -ForegroundColor Cyan

$regSb = New-Object System.Text.StringBuilder
[void]$regSb.AppendLine("#pragma once")
[void]$regSb.AppendLine("#include <Arduino.h>")
[void]$regSb.AppendLine()

# Thêm SampleGif nếu có
if (Test-Path "include/SampleGif.h") {
    [void]$regSb.AppendLine('#include "SampleGif.h"')
}

foreach ($e in $registryEntries) {
    [void]$regSb.AppendLine("#include `"$($e.Header)`"")
}

[void]$regSb.AppendLine()
[void]$regSb.AppendLine("struct GifEntry {")
[void]$regSb.AppendLine("    const char* name;")
[void]$regSb.AppendLine("    const uint8_t* data;")
[void]$regSb.AppendLine("    size_t size;")
[void]$regSb.AppendLine("    uint8_t defaultScale;")
[void]$regSb.AppendLine("};")
[void]$regSb.AppendLine()
[void]$regSb.AppendLine("const GifEntry GIF_REGISTRY[] = {")

foreach ($e in $registryEntries) {
    [void]$regSb.AppendLine("    { `"$($e.Name)`", $($e.VarName), $($e.VarName)_size, $($e.Scale) },")
}

if (Test-Path "include/SampleGif.h") {
    [void]$regSb.AppendLine("    { `"star`", sample_star_gif, sample_star_gif_size, 8 }")
}

[void]$regSb.AppendLine("};")
[void]$regSb.AppendLine()
[void]$regSb.AppendLine("const size_t GIF_REGISTRY_COUNT = sizeof(GIF_REGISTRY) / sizeof(GIF_REGISTRY[0]);")
[void]$regSb.AppendLine()
[void]$regSb.AppendLine("inline const GifEntry* findGifByName(const char* name) {")
[void]$regSb.AppendLine("    if (!name) return nullptr;")
[void]$regSb.AppendLine("    for (size_t i = 0; i < GIF_REGISTRY_COUNT; i++) {")
[void]$regSb.AppendLine("        if (strcasecmp(GIF_REGISTRY[i].name, name) == 0) {")
[void]$regSb.AppendLine("            return &GIF_REGISTRY[i];")
[void]$regSb.AppendLine("        }")
[void]$regSb.AppendLine("    }")
[void]$regSb.AppendLine("    return nullptr;")
[void]$regSb.AppendLine("}")

[System.IO.File]::WriteAllText($regPath, $regSb.ToString())

Write-Host "=======================================================" -ForegroundColor Green
Write-Host "  THANH CONG! Da tao $($registryEntries.Count) header va cap nhat GifRegistry.h!" -ForegroundColor Green
Write-Host "=======================================================" -ForegroundColor Green
