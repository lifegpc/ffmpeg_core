@ECHO OFF
SETLOCAL
SET TOP=%CD%
SET SCRIPTS_DIR=%CD%\scripts
SET DOWNLOAD_RESOURCE=%SCRIPTS_DIR%\download_resource.bat
SET VERSION=v0.0.2
SET FILE=ldd-x86_64-msvc-%VERSION%.7z
CALL %DOWNLOAD_RESOURCE% -o "%FILE%" "https://github.com/lifegpc/ldd-rust/releases/download/%VERSION%/%FILE%" || EXIT %ERRORLEVEL%
7z e "%FILE%" ldd.exe || EXIT %ERRORLEVEL%
MOVE /Y ldd.exe ldd-rust.exe || EXIT %ERRORLEVEL%
ENDLOCAL
