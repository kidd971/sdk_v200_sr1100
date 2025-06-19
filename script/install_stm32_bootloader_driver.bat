@echo off & setlocal

:: If not already running elevated, self-elevate and exit.
:: Note: A helper variable must be used to capture %*,
::       because passing %* to a subroutine doubles ^ chars.
call :IsAdmin || set args=%* && (call :Elevate & exit /b)

:: Getting here means this is (now) an elevated session.
echo "Running in elevated privilege mode."

set POWERSHELL=%SYSTEMROOT%\System32\WindowsPowerShell\v1.0\powershell.exe

echo "Downloading qmk_driver_installer"
set QMK_URI=https://github.com/qmk/qmk_driver_installer/releases/download/v1.01/qmk_driver_installer.exe
set QMK_EXE_PATH=%~dp0\qmk_driver_installer.exe
call %POWERSHELL% -Command Set-Variable ProgressPreference SilentlyContinue; Invoke-Webrequest -URI %QMK_URI% -OutFile %QMK_EXE_PATH%

echo "Running qmk_driver_installer"
%QMK_EXE_PATH% --force --all %~dp0\qmk_drivers\qmk_drivers.txt

echo "Removing qmk_driver_installer"
del %QMK_EXE_PATH%

:: Sleep for 2 second to allow user to read cmd prompt.
timeout 2 > NUL

:: End of the main body.
goto :EOF

:: === Helper subroutines

:: Test if this session is elevated.
:: `net session` only succeeds and therefore reports exit code 0
:: in an elevated session.
:IsAdmin
  net session >NUL 2>&1
goto :EOF

 :: Perform self-elevation, passing all arguments through.
:Elevate
  if defined args set args=%args:^=^^%
  if defined args set args=%args:<=^<%
  if defined args set args=%args:>=^>%
  if defined args set args=%args:&=^&%
  if defined args set args=%args:|=^|%
  if defined args set "args=%args:"=\"\"%"
  :: Note:
  ::  * To not make the non-elevated incarnation wait for the
  ::    elevated one to complete, remove -Wait
  ::  * To keep the elevated session open until explicitly exited by the user,
  ::    use /k instead of /c
  powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    " Start-Process -Wait -Verb RunAs -FilePath cmd -ArgumentList \"/c \"\" cd /d \"\"%CD% \"\" ^&^& \"\"%~f0\"\" %args% \"\" \" "
goto :EOF
