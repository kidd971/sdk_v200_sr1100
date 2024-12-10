@echo off
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

for %%A in ("%~dp0.") do set ACTIVATEPATH=%%~dpA

set MAMBA_ROOT_PREFIX=%ACTIVATEPATH%.micromamba
set PROJECT_ROOT=%ORIGDIR%

if not defined NO_BANNER (
    @REM Print banner
    for /f "delims=: tokens=*" %%A in ('findstr /b ::: "%~f0"') do @echo(%%A
)

@REM Check if batch file is launched from the Command Prompt
echo %CMDCMDLINE% | findstr /c:" /c " > nul && (
    echo Command argument detected.
    echo If using PowerShell, please run activate.ps1.
)

@REM Read project name from environment file
for /d %%D in ("%MAMBA_ROOT_PREFIX%\envs\*") do (
    set "PROJECT_NAME=%%~nxD"
)

echo Initialize virtual environment : %PROJECT_NAME%

call %MAMBA_ROOT_PREFIX%\condabin\mamba_hook.bat

call micromamba activate %PROJECT_NAME%

if not %errorlevel% == 0 (
    echo "Virtual environment not found. Please run bootstrap.bat."
)

exit /b 0
