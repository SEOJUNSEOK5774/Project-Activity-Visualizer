@echo off
setlocal
where py >nul 2>nul
if errorlevel 1 (
    python -B "%~dp0activity_api.py" --demo --port 8766
) else (
    py -3 -B "%~dp0activity_api.py" --demo --port 8766
)
if errorlevel 1 (
    echo Check the message above and see README.md.
    pause
    exit /b 1
)
