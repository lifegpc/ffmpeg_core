@ECHO OFF
SETLOCAL
SET TOP=%CD%
SET PREFIX=%CD%\clib
SET PATH=%PREFIX%\bin;%PATH%
SET INSTALL_PREFIX=%CD%\lib
SET INCLUDE=%INCLUDE%;%PREFIX%\include
IF NOT EXIST build (
    MD build || EXIT /B 1
)
CD build || EXIT /B 1
cmake ^
    -DCMAKE_PREFIX_PATH=%PREFIX% ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_INSTALL_PREFIX=%INSTALL_PREFIX% ^
    -DENABLE_WASAPI=OFF ^
    ../ || EXIT /B %ERRORLEVEL%
cmake --build . --config Debug && cmake --build . --config Debug --target INSTALL ^
    || cmake --build . --config Debug && cmake --build . --config Debug --target INSTALL ^
    || EXIT /B %ERRORLEVEL%
ENDLOCAL
