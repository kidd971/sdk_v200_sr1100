@echo off
setlocal ENABLEDELAYEDEXPANSION
@REM Enable UTF-8 characters.
call chcp 65001 >nul 2>nul
@REM BANNER
:::
:::     ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⠂⠀
:::     ⠀⠀⠀⣠⣴⠾⠟⠛⠛⢉⣠⣾⣿⠃⠀⠀
:::     ⠀⣠⡾⠋⠀⠀⢀⣠⡶⠟⢉⣾⠛⢷⣄⠀     ███████╗██████╗  █████╗ ██████╗ ██╗  ██╗
:::     ⣰⡟⠀⢀⣠⣶⠟⠉⠀⢀⣾⠇⠀⠀⢻⣆     ██╔════╝██╔══██╗██╔══██╗██╔══██╗██║ ██╔╝
:::     ⣿⠁⠀⠉⠛⠿⢶⣤⣀⠈⠋⠀⠀⠀⠈⣿     ███████╗██████╔╝███████║██████╔╝█████╔╝
:::     ⣿⡀⠀⠀⠀⣠⡄⠈⠙⠻⢶⣤⣄⠀⢀⣿     ╚════██║██╔═══╝ ██╔══██║██╔══██╗██╔═██╗
:::     ⠸⣧⡀⠀⣰⡿⠀⠀⣠⣴⠿⠋⠀⠀⣼⠏     ███████║██║     ██║  ██║██║  ██║██║  ██╗
:::     ⠀⠙⢷⣤⡿⣡⣴⠿⠋⠀⠀⢀⣠⡾⠋⠀     ╚══════╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
:::     ⠀⠀⢠⣿⠿⠋⣁⣤⣤⣶⠶⠟⠋⠀⠀⠀     MICROSYSTEMS
:::     ⠀⠠⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
:::

set POWERSHELL=%SYSTEMROOT%\System32\WindowsPowerShell\v1.0\powershell.exe
for %%A in ("%~dp0.") do set ORIGDIR=%%~dpA
set MAMBA_ROOT_PREFIX=%ORIGDIR%\.micromamba
set MICROMAMBAEXE=%MAMBA_ROOT_PREFIX%\micromamba.exe
set _7ZREXE=%MAMBA_ROOT_PREFIX%\Library\bin\7zr.exe
set _7ZA7Z=%MAMBA_ROOT_PREFIX%\Library\bin\7z2201-extra.7z
set _7ZAEXE=%MAMBA_ROOT_PREFIX%\Library\bin\7z-extra\7za.exe
set PROJECT_ROOT=%ORIGDIR%
set YML_FILE=%ORIGDIR%\script\environment.yml
set ENV_FILE_PATH=environment.yml

copy /Y "%YML_FILE%" "%ENV_FILE_PATH%"
if errorlevel 1 (
    echo "ERROR: The file %YML_FILE% could not be copied to %ENV_FILE_PATH%."
    exit /b 1
)

@REM Read the environment name from the yaml file
for /F "usebackq tokens=1,2 delims=: " %%A in ("%ENV_FILE_PATH%") do (
    if "%%A"=="name" (
        set "PROJECT_NAME=%%B"
    )
)

@REM Create virtual environment folder
if not exist "%MAMBA_ROOT_PREFIX%\Library\bin" mkdir %MAMBA_ROOT_PREFIX%\Library\bin

if not exist %_7ZREXE% (
    echo Downloading 7zip executable ...
    call %POWERSHELL% -Command Set-Variable ProgressPreference SilentlyContinue; Invoke-Webrequest -URI https://www.7-zip.org/a/7zr.exe -OutFile %_7ZREXE%
)

if not exist %_7ZAEXE% (
    echo Downloading 7zip extra ...
    call %POWERSHELL% -Command Set-Variable ProgressPreference SilentlyContinue; Invoke-Webrequest -URI https://www.7-zip.org/a/7z2201-extra.7z -OutFile %_7ZA7Z%

    if not exist %_7ZA7Z% (
        echo Error : 7zip extra download failed
        exit /B 1
    )

    echo Extracting 7zip extra ...
    %_7ZREXE% x %_7ZA7Z% -o%MAMBA_ROOT_PREFIX%\Library\bin\7z-extra  >nul 2>&1
    del %_7ZA7Z%
)

@REM Check for missing DLLs required for micromamba and install if not found
set "DLLs=msvcp140.dll vcruntime140.dll vcruntime140_1.dll"
set "SysWOW64=%windir%\SysWOW64"
set "System32=%windir%\System32"

for %%A in (%DLLs%) do (
    if not exist "%System32%\%%A" (
        if not exist "%SysWOW64%\%%A" (
            call %POWERSHELL% -Command Set-Variable ProgressPreference SilentlyContinue; Invoke-Webrequest -URI https://aka.ms/vs/17/release/vc_redist.x64.exe -OutFile %MAMBA_ROOT_PREFIX%\VC_redist.x64.exe

            echo Installing Microsoft Visual C++ Redistributable, please allow the installation to continue ...
            %MAMBA_ROOT_PREFIX%\VC_redist.x64.exe /install /quiet /norestart
            if errorlevel 1 (
                echo Error : Microsoft Visual C++ Redistributable installation failed
                exit /B 1
            )

            del %MAMBA_ROOT_PREFIX%\VC_redist.x64.exe
            goto :breakVCRedist
        )
    )
)
:breakVCRedist

if not exist %MICROMAMBAEXE% (
    echo Downloading Micromamba ...
    pushd .
    cd %MAMBA_ROOT_PREFIX%
    call %POWERSHELL% -Command Set-Variable ProgressPreference SilentlyContinue; Invoke-Webrequest -URI https://micro.mamba.pm/api/micromamba/win-64/1.5.9 -OutFile %ORIGDIR%\micromamba.tar.bz2

    echo Extracting micromamba ...
    %_7ZAEXE% x %ORIGDIR%\micromamba.tar.bz2 -aoa  >nul 2>&1
    del %ORIGDIR%\micromamba.tar.bz2
    %_7ZAEXE% x micromamba.tar -ttar -aoa -r Library\bin\micromamba.exe  >nul 2>&1
    move Library\bin\micromamba.exe %MICROMAMBAEXE%
    del %MAMBA_ROOT_PREFIX%\micromamba.tar

    popd

    @REM Initialize shell activation script
    if not exist %MICROMAMBAEXE% (
        echo Error : micromamba install failed!
        exit /B 1
    )

    call %MICROMAMBAEXE% -q shell hook --shell=cmd.exe

    if not exist %MAMBA_ROOT_PREFIX%/.condarc (
        @REM Disable micromamba banner and set terminal prompt modifier
        echo env_prompt: ^"^(%PROJECT_NAME%^)^"  > %MAMBA_ROOT_PREFIX%/.condarc
        echo show_banner: false >> %MAMBA_ROOT_PREFIX%/.condarc
    )
)

@REM Check if Windows long paths support is enabled
set "KEY=HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem"
set "VALUE=LongPathsEnabled"

for /f "usebackq tokens=*" %%A in (`%POWERSHELL% -Command "(Get-ItemProperty -Path '%KEY%' -Name '%VALUE%').%VALUE% -eq 1"`) do (
    set "LongPathsSupport=%%A"
)

if "%LongPathsSupport%" == "False" (
    @REM Must be run with administrative privileges
    set /P Confirmation="Elevate privileges to enable long path support? (Y/N): "
    if /I "!Confirmation!"=="Y" (
        echo Enabling Windows long paths...
        call %POWERSHELL% -Command "Start-Process powershell -ArgumentList 'New-ItemProperty -Path %KEY% -Name %VALUE% -Value 1 -PropertyType DWORD -Force' -Verb RunAs"
    ) else (
        echo WARNING : You chose not to proceed with the correction process. This may lead to compilation errors. Run this script again to correct this issue later.
    )
)

echo Initialize virtual environment ...

if not defined NO_BANNER (
    @REM Print banner
    for /f "delims=: tokens=*" %%A in ('findstr /b ::: "%~f0"') do @echo(%%A
)

call %MAMBA_ROOT_PREFIX%\condabin\mamba_hook.bat
if not exist %MICROMAMBAEXE% (
    echo Error : micromamba install failed!
    exit /B 1
)

echo. > "%MAMBA_ROOT_PREFIX%\.env.%PROJECT_NAME%"

call micromamba create --name %PROJECT_NAME% -y --file %ENV_FILE_PATH%
call micromamba clean --all --yes
call micromamba activate %PROJECT_NAME%

if not exist "%MAMBA_ROOT_PREFIX%\packs" mkdir %MAMBA_ROOT_PREFIX%\packs

if not exist "%MAMBA_ROOT_PREFIX%\packs\Keil.STM32U5xx_DFP.2.1.0.pack" (
    echo "Download STM32U5 pack"
    call %POWERSHELL% -Command Set-Variable ProgressPreference SilentlyContinue; Invoke-Webrequest -URI https://keilpack.azureedge.net/pack/Keil.STM32U5xx_DFP.2.1.0.pack -OutFile %MAMBA_ROOT_PREFIX%\packs\Keil.STM32U5xx_DFP.2.1.0.pack
)

@REM Check for AutoRun registry issues
set "KEY=HKEY_CURRENT_USER\SOFTWARE\Microsoft\Command Processor"
set "VALUE=AutoRun"

for /F "tokens=2*" %%A in ('reg query "%KEY%" /v "%VALUE%" 2^>nul') do set "AutoRun=%%B"

set "Failed=0"

if defined AutoRun (
    for %%Q in (!AutoRun!) do (
        cmd /C "%%~Q" && (
            set "NewAutoRun=!NewAutoRun! "%%~Q""
        ) || (
            set "Failed=1"
        )
    )
    if "!Failed!"=="1" (
        if not "!NewAutoRun!" == "" (
            set "NewAutoRun=!NewAutoRun:~1!"
        )
        echo Your AutoRun registry entry appears to contain invalid commands, which may lead to compilation errors.
        echo.
        echo Original AutoRun: !AutoRun!
        echo Proposed New AutoRun: !NewAutoRun!
        echo.
        echo Please backup the "HKEY_CURRENT_USER\SOFTWARE\Microsoft\Command Processor\AutoRun" registry before proceeding
        set /P Confirmation="Would you like to proceed with the automatic correction process? (Y/N): "
        if /I "!Confirmation!"=="Y" (
            echo Replacing AutoRun commands...
            reg add "%KEY%" /v "%VALUE%" /d "!NewAutoRun!" /f
        ) else (
            echo WARNING : You chose not to proceed with the correction process. This may lead to compilation errors. Run this script again to correct this issue later.
        )
    )
)

rem Check if .patch files exist in the specified directory
dir /b "%ORIGDIR%\script\*win.patch" 2>nul
if %ERRORLEVEL% equ 0 (
    rem Iterate over each .patch file in the directory and apply using git apply
    for %%f in ("%ORIGDIR%\script\*win.patch") do (
        if exist "%%f" (
            echo Applying patch: "%%f"
            git apply "%%f"
        )
    )
)

endlocal

exit /B 0
