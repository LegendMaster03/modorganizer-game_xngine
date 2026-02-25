$path = 'f:\SteamLibrary\steamapps\common\The Elder Scrolls Adventures Redguard\Redguard\ENGLISH.RTX'
$bytes = [IO.File]::ReadAllBytes($path)
function ReadU16BE([byte[]]$b,[int]$o){ ($b[$o] -shl 8) -bor $b[$o+1] }
function ReadI32LE([byte[]]$b,[int]$o){ [BitConverter]::ToInt32($b,$o) }
$pos=0; $entries=@{}
while($pos+10 -le $bytes.Length){
  $label=[Text.Encoding]::ASCII.GetString($bytes,$pos,4)
  if($label -eq 'END '){break}
  $pos+=4; $len=ReadI32LE $bytes $pos; $pos+=4; $has=ReadU16BE $bytes $pos; $pos+=2; $sl=ReadI32LE $bytes $pos; $pos+=4
  if($sl -lt 0 -or $pos+$sl -gt $bytes.Length){break}
  $sub=[Text.Encoding]::GetEncoding('ISO-8859-1').GetString($bytes,$pos,$sl); $pos+=$sl
  $entries[$label]=$sub
  if($has -eq 1){ $al=ReadI32LE $bytes ($pos+22); $pos += 27 + [Math]::Max(0,$al) }
}
foreach($k in @('HBBL','HBDO','HBCL','NCPL','?smk','?sen','?drt','?leg','?isz','?bye')){ if($entries.ContainsKey($k)){ "$k => $($entries[$k])" } else { "$k => <none>" } }
