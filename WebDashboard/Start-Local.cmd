@echo off
setlocal DisableDelayedExpansion

set "PAV_DATABASE=%~1"
if not defined PAV_DATABASE (
    echo Drag an Unreal project .uproject or activity.db file onto this launcher.
    pause
    exit /b 1
)

if /i not "%~x1"==".uproject" goto :checkDatabase
if not exist "%~f1" (
    echo Unreal project not found at "%~f1".
    pause
    exit /b 1
)
set "PAV_DATABASE=%~dp1Saved\ProjectActivity\activity.db"

:checkDatabase
if not exist "%PAV_DATABASE%" (
    echo Activity database not found at "%PAV_DATABASE%".
    echo Open the project with Project Activity Visualizer enabled and save an asset.
    echo Then drag the .uproject or activity.db file onto this launcher.
    pause
    exit /b 1
)

echo Using activity database "%PAV_DATABASE%".
where py >nul 2>nul
if errorlevel 1 (
    python -B "%~dp0activity_api.py" --db "%PAV_DATABASE%" --open-browser
) else (
    py -3 -B "%~dp0activity_api.py" --db "%PAV_DATABASE%" --open-browser
)

if errorlevel 1 (
    echo Check the message above and see README.md.
    pause
    exit /b 1
)
