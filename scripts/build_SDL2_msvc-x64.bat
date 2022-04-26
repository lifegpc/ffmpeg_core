@ECHO OFF
SETLOCAL
SET PREFIX=%CD%\clib
SET PATH=%PREFIX%\bin;%PATH%
IF NOT EXIST cbuild (
    MD cbuild || EXIT /B 1
)
CD cbuild || EXIT /B 1
git clone --depth 1 https://github.com/libsdl-org/SDL || EXIT /B %ERRORLEVEL%
CD SDL || EXIT /B 1
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
