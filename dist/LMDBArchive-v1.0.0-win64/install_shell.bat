@echo off
echo Installing LMDBArchive shell integration...
echo.

:: Run the GUI with --register-shell flag
"%~dp0LMDBArchive.exe" --register-shell
