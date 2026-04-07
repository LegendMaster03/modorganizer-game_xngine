@echo off
REM Copy this file to local.env.bat and set paths for your machine.
REM This file is not committed to source control.

REM Required
set "MO2_UIBASE_PATH="
set "MO2_UIBASE_LIB="
set "MO2_SRC_PATH="
set "QT_ROOT="

REM Optional (leave blank to use defaults where possible)
set "MO2_PLUGINS_DIR="
set "MO2_TARGET="
REM Optional dual-target overrides. When MO2_TARGET is set to retail or dev,
REM these values override the unsuffixed variables above.
set "MO2_UIBASE_PATH_RETAIL="
set "MO2_UIBASE_LIB_RETAIL="
set "MO2_SRC_PATH_RETAIL="
set "MO2_PLUGINS_DIR_RETAIL="
set "MO2_INSTALL_DIR_RETAIL="
set "BUILD_DIR_RETAIL="
set "MO2_UIBASE_PATH_DEV="
set "MO2_UIBASE_LIB_DEV="
set "MO2_SRC_PATH_DEV="
set "MO2_PLUGINS_DIR_DEV="
set "MO2_INSTALL_DIR_DEV="
set "BUILD_DIR_DEV="
set "QT_ROOT_RETAIL="
set "QT_ROOT_DEV="
set "VCPKG_ROOT="
set "CMAKE_EXE=cmake"
set "NINJA_EXE=ninja"
set "VCVARS_BAT="
