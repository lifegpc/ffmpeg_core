@ECHO OFF
SETLOCAL
SET PREFIX=%CD%\clib
SET PATH=%PREFIX%\bin;%PATH%
IF NOT EXIST cbuild (
    MD cbuild || EXIT /B 1
)
CD cbuild || EXIT /B 1
curl -L "https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.tar.gz" -o SDL2.tar.gz || EXIT /B %ERRORLEVEL%
tar -xzvf SDL2.tar.gz || EXIT /B %ERRORLEVEL%
CD SDL2-* || EXIT /B 1
IF NOT EXIST build (
    MD build || EXIT /B 1
)
CD build || EXIT /B 1
cmake ^
    -DCMAKE_PREFIX_PATH=%PREFIX% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=%PREFIX% ^
    -DSDL_SHARED=ON ^
    -DSDL_STATIC=OFF ^
    ../ || EXIT /B %ERRORLEVEL%
cmake --build . --config Release && cmake --build . --config Release --target INSTALL ^
    || cmake --build . --config Release && cmake --build . --config Release --target INSTALL ^
    || EXIT /B %ERRORLEVEL%
ENDLOCAL
