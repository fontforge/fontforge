#!/bin/bash

# FontForge macOS dependency checker and installer
# This script checks for required dependencies and offers to install missing ones via Homebrew

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Arrays to track missing dependencies
MISSING_BREW=()
MISSING_OTHER=()

echo -e "${BLUE}FontForge macOS Dependency Checker${NC}"
echo "===================================="
echo ""

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}Error: This script is designed for macOS only.${NC}"
    exit 1
fi

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check if a brew formula is installed
brew_installed() {
    brew list "$1" >/dev/null 2>&1
}

# Function to check for pkg-config package
pkg_config_exists() {
    pkg-config --exists "$1" 2>/dev/null
}

# Check for Homebrew
echo -e "${YELLOW}Checking for Homebrew...${NC}"
if ! command_exists brew; then
    echo -e "${RED}Homebrew is not installed!${NC}"
    echo "Please install Homebrew first: https://brew.sh"
    echo "Run: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi
echo -e "${GREEN}✓ Homebrew is installed${NC}"
echo ""

# Check for Xcode Command Line Tools
echo -e "${YELLOW}Checking for Xcode Command Line Tools...${NC}"
if ! xcode-select -p >/dev/null 2>&1; then
    echo -e "${RED}Xcode Command Line Tools are not installed!${NC}"
    echo "Installing Xcode Command Line Tools..."
    xcode-select --install
    echo "Please complete the installation and run this script again."
    exit 1
fi
echo -e "${GREEN}✓ Xcode Command Line Tools are installed${NC}"
echo ""

# Define required dependencies
echo -e "${YELLOW}Checking required dependencies...${NC}"

# Build tools
BUILD_TOOLS=(
    "cmake:CMake build system"
    "ninja:Ninja build tool"
    "pkg-config:Package configuration tool"
    "gettext:GNU internationalization utilities"
)

# Required libraries
REQUIRED_LIBS=(
    "freetype:Font rendering library"
    "glib:Core application building blocks"
    "pango:Text layout library"
    "cairo:2D graphics library"
    "libxml2:XML parsing library"
)

# GUI dependencies (optional but usually desired)
GUI_LIBS=(
    "gtk+3:GTK+ 3 GUI toolkit"
    "gtkmm3:C++ interfaces for GTK+ and GNOME"
)

# Optional but recommended libraries
OPTIONAL_LIBS=(
    "libspiro:Spiro spline support"
    "giflib:GIF image support"
    "jpeg:JPEG image support"
    "libpng:PNG image support"
    "libtiff:TIFF image support"
    "libuninameslist:Unicode character names"
    "woff2:WOFF2 font format support"
    "python@3:Python scripting support"
    "readline:Command line editing"
)

# Additional useful tools
ADDITIONAL_TOOLS=(
    "coreutils:GNU core utilities"
    "fontconfig:Font configuration library"
    "libtool:Generic library support"
    "wget:Network downloader"
)

# Function to check a dependency
check_dependency() {
    local package=$1
    local description=$2
    local is_optional=$3
    
    if brew_installed "$package"; then
        echo -e "${GREEN}✓ $package${NC} - $description"
    else
        if [[ "$is_optional" == "true" ]]; then
            echo -e "${YELLOW}○ $package${NC} - $description (optional)"
        else
            echo -e "${RED}✗ $package${NC} - $description"
        fi
        MISSING_BREW+=("$package")
    fi
}

# Check build tools
echo ""
echo "Build Tools:"
for tool in "${BUILD_TOOLS[@]}"; do
    IFS=':' read -r package description <<< "$tool"
    check_dependency "$package" "$description" "false"
done

# Check required libraries
echo ""
echo "Required Libraries:"
for lib in "${REQUIRED_LIBS[@]}"; do
    IFS=':' read -r package description <<< "$lib"
    check_dependency "$package" "$description" "false"
done

# Check GUI libraries
echo ""
echo "GUI Libraries:"
for lib in "${GUI_LIBS[@]}"; do
    IFS=':' read -r package description <<< "$lib"
    check_dependency "$package" "$description" "false"
done

# Check optional libraries
echo ""
echo "Optional Libraries (recommended):"
for lib in "${OPTIONAL_LIBS[@]}"; do
    IFS=':' read -r package description <<< "$lib"
    check_dependency "$package" "$description" "true"
done

# Check additional tools
echo ""
echo "Additional Tools:"
for tool in "${ADDITIONAL_TOOLS[@]}"; do
    IFS=':' read -r package description <<< "$tool"
    check_dependency "$package" "$description" "true"
done

# Special checks
echo ""
echo -e "${YELLOW}Performing special checks...${NC}"

# Check Python version
if command_exists python3; then
    PYTHON_VERSION=$(python3 -c 'import sys; print(".".join(map(str, sys.version_info[:2])))')
    PYTHON_MAJOR=$(echo $PYTHON_VERSION | cut -d. -f1)
    PYTHON_MINOR=$(echo $PYTHON_VERSION | cut -d. -f2)
    
    if [[ $PYTHON_MAJOR -ge 3 ]] && [[ $PYTHON_MINOR -ge 8 ]]; then
        echo -e "${GREEN}✓ Python $PYTHON_VERSION${NC} (>= 3.8 required)"
    else
        echo -e "${YELLOW}○ Python $PYTHON_VERSION${NC} (>= 3.8 recommended for full functionality)"
    fi
else
    echo -e "${YELLOW}○ Python 3${NC} not found (optional but recommended)"
fi

# Check for Sphinx (Python documentation generator)
echo ""
echo -e "${YELLOW}Checking for Python packages...${NC}"
if command_exists python3; then
    if python3 -c "import sphinx" 2>/dev/null; then
        SPHINX_VERSION=$(python3 -c "import sphinx; print(sphinx.__version__)")
        echo -e "${GREEN}✓ Sphinx $SPHINX_VERSION${NC} - Documentation generator"
    else
        echo -e "${YELLOW}○ Sphinx${NC} - Documentation generator (optional, needed to build docs)"
        echo "  To install: pip3 install sphinx"
    fi
else
    echo -e "${YELLOW}○ Sphinx${NC} - Requires Python 3 to be installed"
fi

# Summary
echo ""
echo "===================================="
if [[ ${#MISSING_BREW[@]} -eq 0 ]]; then
    echo -e "${GREEN}All dependencies are installed!${NC}"
    echo ""
    echo "You can now build FontForge with:"
    echo "  mkdir build"
    echo "  cd build"
    echo "  cmake -GNinja .."
    echo "  ninja"
    echo "  sudo ninja install"
else
    echo -e "${YELLOW}Missing dependencies found.${NC}"
    echo ""
    echo "The following packages are not installed:"
    for pkg in "${MISSING_BREW[@]}"; do
        echo "  - $pkg"
    done
    
    echo ""
    read -p "Would you like to install the missing dependencies? (y/N) " -n 1 -r
    echo ""
    
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${BLUE}Installing missing dependencies...${NC}"
        
        # Update Homebrew
        echo "Updating Homebrew..."
        brew update
        
        # Install missing packages
        for pkg in "${MISSING_BREW[@]}"; do
            echo -e "${BLUE}Installing $pkg...${NC}"
            brew install "$pkg" || echo -e "${YELLOW}Warning: Failed to install $pkg${NC}"
        done
        
        # Special handling for gettext
        if [[ " ${MISSING_BREW[@]} " =~ " gettext " ]]; then
            echo -e "${BLUE}Linking gettext...${NC}"
            brew link gettext --force || true
        fi
        
        echo ""
        echo -e "${GREEN}Installation complete!${NC}"
        echo ""
        echo "You may need to add some paths to your environment:"
        echo "  export PATH=\"/usr/local/opt/gettext/bin:\$PATH\""
        echo "  export PKG_CONFIG_PATH=\"/usr/local/opt/libffi/lib/pkgconfig:\$PKG_CONFIG_PATH\""
        echo ""
        echo "You can now build FontForge with:"
        echo "  mkdir build"
        echo "  cd build"
        echo "  cmake -GNinja .."
        echo "  ninja"
        echo "  sudo ninja install"
    else
        echo ""
        echo "To install the missing dependencies manually, run:"
        echo -e "${BLUE}brew install ${MISSING_BREW[*]}${NC}"
    fi
fi

echo ""
echo "For more information, see INSTALL.md"