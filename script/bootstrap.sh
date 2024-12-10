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
PATH="$HOME/.local/bin:$PATH"

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

set_default=false
RELATIVE_ENV_FILE_PATH=""

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
         echo "Usage: source bootstrap.sh [filename] [-d | --default] [-h | --help]"
         echo "Optional arguments:"
         echo "  filename: YML file to use for environment creation"
         echo "  -d | --default: Set the new environment as the default one"
         echo "  -h | --help: Show this help message"
         return 0
         ;;
        -d|--default)
            set_default=true
            shift
            ;;
        *)
            RELATIVE_ENV_FILE_PATH="$1"
            shift
            ;;
    esac
done

DEFAULT_ENV_FILE_PATH=$SOURCE/.micromamba/etc/profile.d/default_env

# check if default environment file exists.
if [ -f $DEFAULT_ENV_FILE_PATH ]; then
   # if yes, read it.
   DEFAULT_ENV_PATH=$(cat $DEFAULT_ENV_FILE_PATH)
else
   # if not, create it.
   if [[ "$OSTYPE" == "darwin"* ]]; then
      echo "macOS detected"
      DEFAULT_ENV_PATH=$SOURCE/script/environment_macos.yml
   else
      DEFAULT_ENV_PATH=$SOURCE/script/environment.yml
   fi
   mkdir -p $SOURCE/.micromamba/etc/profile.d
   echo $DEFAULT_ENV_PATH > $DEFAULT_ENV_FILE_PATH
fi

ENV_FILE_PATH="$SOURCE/environment.yml"

if [[ -n "$RELATIVE_ENV_FILE_PATH" ]]; then
   # if environment file is specified, use it.
   YML_FILE="$(realpath "$RELATIVE_ENV_FILE_PATH")"
else
   # if not, use the default one.
   YML_FILE=$DEFAULT_ENV_PATH
fi

# Copy the yml file to the environment file
cp "$YML_FILE" "$ENV_FILE_PATH"

if [ $? -ne 0 ]; then
    echo "ERROR: The file $YML_FILE could not be copied to $ENV_FILE_PATH."
    exit 1
fi

# Read project name from environment file.
PROJECT_NAME="$( grep '^name:' $ENV_FILE_PATH | sed 's/^name: //' )"

if [ -z "$PROJECT_NAME" ]; then
   echo 'Error : Unable to detect project name. Please check the environment file.'
   return 1
fi

if $set_default; then
   # if default environment is set, update the default environment file.
  echo $ENV_FILE_PATH > $DEFAULT_ENV_FILE_PATH
fi

OS=$(uname -s | tr '[:upper:]' '[:lower:]')

export MAMBA_ROOT_PREFIX="$SOURCE/.micromamba"
export MAMBA_EXE="$MAMBA_ROOT_PREFIX/micromamba"

if [ -f $MAMBA_ROOT_PREFIX/etc/profile.d/micromamba.sh ]; then
   source $MAMBA_ROOT_PREFIX/etc/profile.d/micromamba.sh
fi

if ! [ -f $MAMBA_ROOT_PREFIX/micromamba ]; then
   mkdir -p $MAMBA_ROOT_PREFIX/etc/profile.d

   echo 'Downloading Micromamba ...'
   cd $MAMBA_ROOT_PREFIX

   if [ $OS = "darwin" ]; then
      if [ "$(uname -m)" = "x86_64" ]; then
         curl -Ls https://micro.mamba.pm/api/micromamba/osx-64/1.5.9 | tar -xvj --strip-components=1 -C . bin/micromamba
      else
         curl -Ls https://micro.mamba.pm/api/micromamba/osx-arm64/1.5.9 | tar -xvj --strip-components=1 -C . bin/micromamba
      fi
   elif [ $OS = "linux" ]; then
      if [ "$(uname -m)" = "x86_64" ]; then
         curl -Ls https://micro.mamba.pm/api/micromamba/linux-64/1.5.9 | tar -xvj --strip-components=1 -C . bin/micromamba
      else
         echo 'Error : Unsupported system'
         return 1
      fi
   else
      echo 'Error : Unsupported system'
      return 1
   fi
   cd -

   eval "$(.micromamba/micromamba shell hook --shell=posix)"
   printf '%s\n' "$(.micromamba/micromamba shell hook --shell=posix)" > $MAMBA_ROOT_PREFIX/etc/profile.d/micromamba.sh

   touch $MAMBA_ROOT_PREFIX/.env.$PROJECT_NAME

   #Disable micromamba banner.
   echo "show_banner: false" >> $MAMBA_ROOT_PREFIX/.condarc
fi

if command -v micromamba &> /dev/null; then
   # if the NO_BANNER variable is not set, display the banner
   if [ -z ${NO_BANNER} ]; then
      echo "Initialize virtual environment : $PROJECT_NAME"
      printf '%b\n' "$BANNER"
   fi


   #init micromamba environment.
   micromamba create -y --file $ENV_FILE_PATH
   micromamba clean --all --yes
   micromamba activate $PROJECT_NAME
else
   echo 'Error : micromamba not installed properly'
   return 1
fi

if [ $OS = "linux" ]; then
   # Recreate gdb symlink due to Pigweed override
   ln -sf $CONDA_PREFIX/arm-none-eabi/bin/arm-none-eabi-gdb $CONDA_PREFIX/bin/arm-none-eabi-gdb
   cd $CONDA_PREFIX/lib/
   ln -sf libtinfo.so.6 libtinfo.so.5
   ln -sf libncurses.so.6 libncurses.so.5
   cd -
fi

if [ -v ${BOOTSTRAP_SKIP_PYOCD} ]; then
   #Check if pyOCD packages are already installed.
   if [ ! -d "$MAMBA_ROOT_PREFIX/packs" ]; then
      mkdir $MAMBA_ROOT_PREFIX/packs
   fi

   if [ ! -f "$MAMBA_ROOT_PREFIX/packs/Keil.STM32U5xx_DFP.2.1.0.pack" ]; then
      echo "Download STM32U5 pack"
      curl -Ls https://keilpack.azureedge.net/pack/Keil.STM32U5xx_DFP.2.1.0.pack --output $MAMBA_ROOT_PREFIX/packs/Keil.STM32U5xx_DFP.2.1.0.pack
   fi
fi

# Check if any .patch file exists in the specified directory
patch_files=$(find "$PROJECT_ROOT/script" -type f -name "*unix.patch")

if [ -n "$patch_files" ]; then
   # Iterate over each .patch file found by find
   for patch_file in $patch_files; do
      if [ -f "$patch_file" ]; then
            echo "Applying patch: $patch_file"
            # Directly apply the patch file without assuming its location
            git apply "$patch_file"
      fi
   done
fi

return 0
