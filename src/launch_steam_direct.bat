@echo off
REM Direct Steam DOSBox launcher - bypasses MO2 VFS to prevent crashes
REM Matches official Steam batch file behavior

cd /d "F:\SteamLibrary\steamapps\common\The Elder Scrolls Adventures Redguard\DOSBox-0.73"
dosbox.exe -noconsole -conf rg.conf %*
