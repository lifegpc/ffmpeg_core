@ECHO OFF
SETLOCAL
SET PREFIX=%CD%\clib
IF NOT EXIST cbuild (
    MD cbuild || EXIT /B 1
)
CD cbuild || EXIT /B 1
git clone --depth 1 "https://github.com/pkgconf/pkgconf" || EXIT /B %ERRORLEVEL%
CD pkgconf || EXIT /B %ERRORLEVEL%
meson setup build --prefix %PREFIX% -Dtests=false || EXIT /B %ERRORLEVEL%
meson compile -C build || EXIT /B %ERRORLEVEL%
meson install -C build || EXIT /B %ERRORLEVEL%
CD %PREFIX%\bin || EXIT /B 1
COPY /Y pkgconf.exe pkg-config.exe || EXIT /B %ERRORLEVEL%
ENDLOCAL
