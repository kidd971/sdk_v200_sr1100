BANNER=\
'

     \u001b[31m⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⠂⠀\u001b[0m
     \u001b[31m⠀⠀⠀⣠⣴⠾⠟⠛⠛⢉⣠⣾⣿⠃⠀⠀\u001b[0m
     \u001b[31m⠀⣠⡾⠋⠀⠀⢀⣠⡶⠟⢉⣾⠛⢷⣄⠀\u001b[0m     ███████╗██████╗  █████╗ ██████╗ ██╗  ██╗
     \u001b[31m⣰⡟⠀⢀⣠⣶⠟⠉⠀⢀⣾⠇⠀⠀⢻⣆\u001b[0m     ██╔════╝██╔══██╗██╔══██╗██╔══██╗██║ ██╔╝
     \u001b[31m⣿⠁⠀⠉⠛⠿⢶⣤⣀⠈⠋⠀⠀⠀⠈⣿\u001b[0m     ███████╗██████╔╝███████║██████╔╝█████╔╝
     \u001b[31m⣿⡀⠀⠀⠀⣠⡄⠈⠙⠻⢶⣤⣄⠀⢀⣿\u001b[0m     ╚════██║██╔═══╝ ██╔══██║██╔══██╗██╔═██╗
     \u001b[31m⠸⣧⡀⠀⣰⡿⠀⠀⣠⣴⠿⠋⠀⠀⣼⠏\u001b[0m     ███████║██║     ██║  ██║██║  ██║██║  ██╗
     \u001b[31m⠀⠙⢷⣤⡿⣡⣴⠿⠋⠀⠀⢀⣠⡾⠋⠀\u001b[0m     ╚══════╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
     \u001b[31m⠀⠀⢠⣿⠿⠋⣁⣤⣤⣶⠶⠟⠋⠀⠀⠀\u001b[0m     MICROSYSTEMS
     \u001b[31m⠀⠠⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\u001b[0m

'
if test -n "$BASH"; then
  SOURCE=$( cd -- "$( dirname -- "$( dirname -- "${BASH_SOURCE[0]}" )" )" &> /dev/null && pwd )
elif test -n "$ZSH_NAME"; then
  SOURCE="$( dirname -- "$( dirname -- "$( readlink -f -- "$0"; )"; )"; )"
else
  echo 'Error : Unable to detect shell. Only bash and zsh are supported'
  return 1
fi

PROJECT_ROOT="$SOURCE"
export PROJECT_ROOT

# Check if environment exists
if [ ! -d "$SOURCE/.micromamba" ]; then
  echo "Error : Environment not found. Please run \"source script/bootstrap.sh\"."
  return 1
fi

# check if there is an argument
if [[ $# -eq 0 ]]; then
  env_dirs=($SOURCE/.micromamba/envs/*)

  # If there is only one virtual environment, use it.
  if [ ${#env_dirs[@]} -eq 1 ]; then
    # Project name is the name of the directory.
    PROJECT_NAME="$(basename $env_dirs)"
  else
    # No argument, use the default environment file.
    DEFAULT_ENV_FILE_PATH=$SOURCE/.micromamba/etc/profile.d/default_env
    ENV_FILE_PATH=$(cat $DEFAULT_ENV_FILE_PATH)
    PROJECT_NAME="$( grep '^name:' $ENV_FILE_PATH | sed 's/^name: //' )"
  fi
else
  # check if the argument is a yaml file
  extension="${1: -4}"
  if [[ $extension == ".yml" ]]; then
    # The argument is the path to the environment file.
    ENV_FILE_PATH="$(realpath "$1")"
    PROJECT_NAME="$( grep '^name:' $ENV_FILE_PATH | sed 's/^name: //' )"
  else
    # The argument is the name of the environment.
    PROJECT_NAME=$1
  fi
fi

export MAMBA_ROOT_PREFIX=$SOURCE/.micromamba
export MAMBA_EXE="$MAMBA_ROOT_PREFIX/micromamba"

if [ -z ${NO_BANNER} ]; then
# Print banner if environment is not already initialized.
  if [[ -z "${CONDA_DEFAULT_ENV}" ]]; then
    echo "Initialize virtual environment : $PROJECT_NAME"
    printf '%b\n' "$BANNER"
  fi
fi

if [ -f $MAMBA_ROOT_PREFIX/etc/profile.d/micromamba.sh ]; then
    source $MAMBA_ROOT_PREFIX/etc/profile.d/micromamba.sh
else
    return 1
fi

if command -v micromamba &> /dev/null; then
    micromamba activate $PROJECT_NAME
else
    echo 'Error : Unable to activate environment.'
    return 1
fi
