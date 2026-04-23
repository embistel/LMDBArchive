@echo off
echo Removing LMDBArchive shell integration...
echo.

:: Run the GUI with --unregister-shell flag
"%~dp0LMDBArchive.exe" --unregister-shell
