@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
SET TOP=%CD%
SET SCRIPTS_DIR=%CD%\scripts
SET DOWNLOAD_RESOURCE=%SCRIPTS_DIR%\download_resource.bat
SET EXTRACT_ZIP=%SCRIPTS_DIR%\extract_zip.bat
SET PREFIX=%CD%\clib
SET VERSION=10.2+2.0.1
SET FILE=libcdio-paranoia_release-%VERSION:+=.%_msvc16.zip
IF NOT EXIST cbuild (
    MD cbuild || EXIT /B 1
)
CD cbuild || EXIT /B 1
CALL %DOWNLOAD_RESOURCE% -o "%FILE%" "https://github.com/ShiftMediaProject/libcdio-paranoia/releases/download/release-!VERSION:+=%%%%2B!/libcdio-paranoia_release-10.2.2.0.1_msvc16.zip" || EXIT /B %ERRORLEVEL%
CALL %EXTRACT_ZIP% "%FILE%" x64 "%PREFIX%" || EXIT /B %ERRORLEVEL%
CD %PREFIX%/lib || EXIT /B 1
IF NOT EXIST pkgconfig (
    MD pkgconfig || EXIT /B 1
)
CD pkgconfig || EXIT /B 1
IF EXIST libcdio-paranoia.pc (
    DEL /Q libcdio-paranoia.pc || EXIT /B 1
)
echo prefix=%PREFIX:\=/% > libcdio-paranoia.pc
echo libdir=${prefix}/lib >> libcdio-paranoia.pc
echo includedir=${prefix}/include >> libcdio-paranoia.pc
echo Name: libcdio-paranoia >> libcdio-paranoia.pc
echo Description: CD paranoia library from libcdio >> libcdio-paranoia.pc
echo Version: %VERSION:+=.% >> libcdio-paranoia.pc
echo Libs: -L${libdir} -lcdio-paranoia >> libcdio-paranoia.pc
echo Cflags: -I${includedir} >> libcdio-paranoia.pc
ENDLOCAL
