@ECHO OFF
SETLOCAL
SET TOP=%CD%
SET PREFIX=%CD%\clib
SET PATH=%PREFIX%\bin;%PATH%
SET INSTALL_PREFIX=%CD%\lib
IF NOT EXIST build (
    MD build || EXIT /B 1
)
CD build || EXIT /B 1
cmake ^
    -DCMAKE_PREFIX_PATH=%PREFIX% ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
    -DCMAKE_INSTALL_PREFIX=%INSTALL_PREFIX% ^
    -DENABLE_WASAPI=ON ^
    ../ || EXIT /B %ERRORLEVEL%
cmake --build . --config RelWithDebInfo && cmake --build . --config RelWithDebInfo --target INSTALL ^
    || cmake --build . --config RelWithDebInfo && cmake --build . --config RelWithDebInfo --target INSTALL ^
    || EXIT /B %ERRORLEVEL%
ENDLOCAL
