param([string]$InputFile, [string]$OutputFile, [string]$VarName)

if (-not (Test-Path $InputFile)) {
    Write-Error "File not found: $InputFile"
    exit 1
}

$bytes = [System.IO.File]::ReadAllBytes($InputFile)
Write-Host "Read $($bytes.Length) bytes from $InputFile"

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("#pragma once")
[void]$sb.AppendLine("#include <Arduino.h>")
[void]$sb.AppendLine()
[void]$sb.AppendLine("const uint8_t ${VarName}[] PROGMEM = {")

for ($i = 0; $i -lt $bytes.Length; $i++) {
    if (($i % 16) -eq 0) {
        if ($i -gt 0) { [void]$sb.AppendLine() }
        [void]$sb.Append("    ")
    }
    [void]$sb.Append(("0x{0:x2}, " -f $bytes[$i]))
}

[void]$sb.AppendLine()
[void]$sb.AppendLine("};")
[void]$sb.AppendLine("const size_t ${VarName}_size = $($bytes.Length);")

[System.IO.File]::WriteAllText($OutputFile, $sb.ToString())
Write-Host "Successfully wrote $OutputFile"
