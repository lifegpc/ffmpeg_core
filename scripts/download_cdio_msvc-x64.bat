@ECHO OFF
SETLOCAL
SET TOP=%CD%
SET SCRIPTS_DIR=%CD%\scripts
SET DOWNLOAD_RESOURCE=%SCRIPTS_DIR%\download_resource.bat
SET EXTRACT_ZIP=%SCRIPTS_DIR%\extract_zip.bat
SET PREFIX=%CD%\clib
SET VERSION=2.1.0-1
SET FILE=libcdio_release-%VERSION%_msvc16.zip
IF NOT EXIST cbuild (
    MD cbuild || EXIT /B 1
)
CD cbuild || EXIT /B 1
CALL %DOWNLOAD_RESOURCE% -o "%FILE%" "https://github.com/ShiftMediaProject/libcdio/releases/download/release-%VERSION%/%FILE%"
CALL %EXTRACT_ZIP% "%FILE%" x64 "%PREFIX%" || EXIT /B %ERRORLEVEL%
ENDLOCAL
