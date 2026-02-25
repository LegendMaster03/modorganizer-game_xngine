$ErrorActionPreference='Stop'
$path = 'f:\SteamLibrary\steamapps\common\The Elder Scrolls Adventures Redguard\Redguard\ENGLISH.RTX'
$bytes = [System.IO.File]::ReadAllBytes($path)
function ReadU16BE([byte[]]$b,[int]$o){ return ($b[$o] -shl 8) -bor $b[$o+1] }
function ReadI32LE([byte[]]$b,[int]$o){ return [BitConverter]::ToInt32($b,$o) }
$pos = 0
$entries = @{}
$count = 0
while ($pos + 10 -le $bytes.Length) {
  $label = [System.Text.Encoding]::ASCII.GetString($bytes, $pos, 4)
  if ($label -eq 'END ') { break }
  $pos += 4
  if ($pos + 6 -gt $bytes.Length) { break }
  $lenLE = ReadI32LE $bytes $pos
  $pos += 4
  $hasAudio = ReadU16BE $bytes $pos
  $pos += 2
  if ($pos + 4 -gt $bytes.Length) { break }
  $subLen = ReadI32LE $bytes $pos
  $pos += 4
  if ($subLen -lt 0 -or $pos + $subLen -gt $bytes.Length) { break }
  $subtitle = [System.Text.Encoding]::GetEncoding('ISO-8859-1').GetString($bytes, $pos, $subLen)
  $pos += $subLen
  $entries[$label] = $subtitle
  $count++
  if ($hasAudio -eq 1) {
    if ($pos + 27 -gt $bytes.Length) { break }
    $audioLen = ReadI32LE $bytes ($pos+22)
    $pos += 27
    if ($audioLen -lt 0 -or $pos + $audioLen -gt $bytes.Length) { break }
    $pos += $audioLen
  }
}
Write-Output "entries=$count pos=$pos len=$($bytes.Length)"
$entries.GetEnumerator() | Where-Object { $_.Value -match 'North Point|Bell Tower|Silversmith|Stros|M''Kai|Sentinel|Harbor|Harbour|District|Inn|Shop' } | Sort-Object Name | Select-Object -First 80 | ForEach-Object { Write-Output ("{0} => {1}" -f $_.Name, $_.Value) }
Write-Output '--- direct keys ---'
foreach($k in @('$082','$084','$001','082 ','084 ','001 ')){
  if($entries.ContainsKey($k)){ Write-Output ("{0} => {1}" -f $k, $entries[$k]) }
}
