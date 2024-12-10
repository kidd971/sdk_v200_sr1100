##############################################################################
##
## activate.ps1
##
## Activates the SPARK virtual environment in a PowerShell terminal.
## bootstrap.bat must be run initially to create the virtual environment.
##
##############################################################################

# Enable UTF-8 characters.
$OutputEncoding = [System.Text.Encoding]::UTF8

$BANNER = @"

     ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⠂⠀
     ⠀⠀⠀⣠⣴⠾⠟⠛⠛⢉⣠⣾⣿⠃⠀⠀
     ⠀⣠⡾⠋⠀⠀⢀⣠⡶⠟⢉⣾⠛⢷⣄⠀     ███████╗██████╗  █████╗ ██████╗ ██╗  ██╗
     ⣰⡟⠀⢀⣠⣶⠟⠉⠀⢀⣾⠇⠀⠀⢻⣆     ██╔════╝██╔══██╗██╔══██╗██╔══██╗██║ ██╔╝
     ⣿⠁⠀⠉⠛⠿⢶⣤⣀⠈⠋⠀⠀⠀⠈⣿     ███████╗██████╔╝███████║██████╔╝█████╔╝
     ⣿⡀⠀⠀⠀⣠⡄⠈⠙⠻⢶⣤⣄⠀⢀⣿     ╚════██║██╔═══╝ ██╔══██║██╔══██╗██╔═██╗
     ⠸⣧⡀⠀⣰⡿⠀⠀⣠⣴⠿⠋⠀⠀⣼⠏     ███████║██║     ██║  ██║██║  ██║██║  ██╗
     ⠀⠙⢷⣤⡿⣡⣴⠿⠋⠀⠀⢀⣠⡾⠋⠀     ╚══════╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
     ⠀⠀⢠⣿⠿⠋⣁⣤⣤⣶⠶⠟⠋⠀⠀⠀     MICROSYSTEMS
     ⠀⠠⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀

"@

# Print banner if NO_BANNER is not defined
if (-not $env:NO_BANNER) {
    Write-Host $BANNER
}

$ACTIVATEPATH = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$env:MAMBA_ROOT_PREFIX = "$ACTIVATEPATH\.micromamba"
$MICROMAMBAEXE = "$env:MAMBA_ROOT_PREFIX\micromamba.exe"

# Check if environment exists
if (-not (Test-Path -Path "$env:MAMBA_ROOT_PREFIX" -PathType Container)) {
    Write-Host "Virtual environment not found. Please run bootstrap.bat."
    exit 1
}

# Read project name from environment file
Get-ChildItem -Directory "$env:MAMBA_ROOT_PREFIX\envs\*" | ForEach-Object {
    $PROJECT_NAME = $_.Name
}

Write-Host "Initialize virtual environment : $PROJECT_NAME"

$shellHookCommand = (& $MICROMAMBAEXE shell hook --shell=powershell -p $env:MAMBA_ROOT_PREFIX | Out-String)
Invoke-Expression -Command $shellHookCommand
Invoke-Mamba activate $PROJECT_NAME

if (-not $?) {
    Write-Host "Micromamba not found. Please run bootstrap.bat."
}
