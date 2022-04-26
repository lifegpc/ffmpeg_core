@ECHO OFF
SETLOCAL
SET TOP=%CD%
SET SCRIPTS_DIR=%CD%\scripts
SET DOWNLOAD_RESOURCE=%SCRIPTS_DIR%\download_resource.bat
SET EXTRACT_ZIP=%SCRIPTS_DIR%\extract_zip.bat
SET PREFIX=%CD%\clib
SET VERSION=3.7.4
SET FILE=libgnutls_%VERSION%_msvc17.zip
IF NOT EXIST cbuild (
    MD cbuild || EXIT /B 1
)
CD cbuild || EXIT /B 1
CALL %DOWNLOAD_RESOURCE% -o %FILE% "https://github.com/ShiftMediaProject/gnutls/releases/download/%VERSION%/%FILE%" || EXIT /B %ERRORLEVEL%
CALL %EXTRACT_ZIP% "%FILE%" x64 "%PREFIX%" || EXIT /B %ERRORLEVEL%
CD %PREFIX%/lib || EXIT /B 1
IF NOT EXIST pkgconfig (
    MD pkgconfig || EXIT /B 1
)
CD pkgconfig || EXIT /B 1
IF EXIST gnutls.pc (
    DEL /Q gnutls.pc || EXIT /B 1
)
echo prefix=%PREFIX:\=/% > gnutls.pc
echo libdir=${prefix}/lib >> gnutls.pc
echo includedir=${prefix}/include >> gnutls.pc
echo Name: gnutls >> gnutls.pc
echo Description: Transport Security Layer implementation for the GNU system >> gnutls.pc
echo URL: https://www.gnutls.org/ >> gnutls.pc
echo Version: %VERSION% >> gnutls.pc
echo Libs: -L${libdir} -lgnutls >> gnutls.pc
echo Cflags: -I${includedir} >> gnutls.pc
ENDLOCAL
