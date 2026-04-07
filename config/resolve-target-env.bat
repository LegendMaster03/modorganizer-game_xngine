@echo off

if "%MO2_TARGET%"=="" goto :eof

call :apply MO2_UIBASE_PATH
call :apply MO2_UIBASE_LIB
call :apply MO2_SRC_PATH
call :apply MO2_PLUGINS_DIR
call :apply MO2_INSTALL_DIR
call :apply QT_ROOT
call :apply BUILD_DIR
goto :eof

:apply
call set "TARGET_VALUE=%%%~1_%MO2_TARGET%%%"
if defined TARGET_VALUE (
  call set "%~1=%%%~1_%MO2_TARGET%%%"
)
goto :eof
