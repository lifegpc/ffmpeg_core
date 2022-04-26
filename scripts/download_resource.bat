@ECHO OFF
SETLOCAL
SET DOWNLOAD_RESOURCE_PY=download_resource.py
IF NOT EXIST %DOWNLOAD_RESOURCE_PY% (
    SET DOWNLOAD_RESOURCE_PY=%CD%\download_resource.py
    IF NOT EXIST %DOWNLOAD_RESOURCE_PY% (
        IF DEFINED SCRIPTS_DIR (
            SET DOWNLOAD_RESOURCE_PY=%SCRIPTS_DIR%\download_resource.py
        )
    )
)
IF NOT EXIST %DOWNLOAD_RESOURCE_PY% (
    EXIT /B 1
)
python %DOWNLOAD_RESOURCE_PY% %*
IF ERRORLEVEL 2 (
    python -m pip install --upgrade requests
    IF %ERRORLEVEL% NEQ 0 (
        EXIT /B 2
    )
    python %DOWNLOAD_RESOURCE_PY% %*
)
IF %ERRORLEVEL% NEQ 0 (
    EXIT /B %ERRORLEVEL%
)
ENDLOCAL
