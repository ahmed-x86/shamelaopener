# Shamela Opener

A C++ and Qt-based graphical utility designed to seamlessly download, install, configure, and run the [Shamela Library](https://shamela.ws/) on Linux environments (specifically tailored for Arch Linux). 

It resolves common runtime and dependency issues by generating a custom wrapper script and provides a modern, customizable user interface.

## 🌟 Features

* **Automated Installation & Setup:** 
  * Download the Shamela Library directly within the app (from archive.org).
  * Automatically extract `.tar.xz` or `.zip` archives.
  * Generates a robust `launch.sh` script to fix Qt and environment variable conflicts (e.g., `LD_LIBRARY_PATH`, `QT_QPA_PLATFORM=xcb`).
* **Dependency Checker:** Built-in checker using `pacman` to verify if required packages (like `freetype2`, `fontconfig`, `glibc`, `zlib`, etc.) are installed on your Arch system.
* **Modern UI & Animations:** Features a glowing background effect, animated sidebar, and smooth ripple click animations on buttons.
* **Extensive Theming System:**
  * Comes with pre-installed popular color palettes: **Catppuccin Mocha, Catppuccin Latte, Shamela Classic, Dracula, Nord, and Gruvbox**.
  * **Custom Theme Engine:** Create your own theme using a built-in color picker (Base, Surface, Text, Accents) with dark mode glow toggles.
* **Bilingual Support:** Fully supports English and Arabic interfaces (RTL/LTR dynamic switching).
* **State Management:** Automatically saves your configurations, custom themes, and library paths to `~/.shamela_path.txt`.

## 🛠️ Build and Run

Make sure you have `cmake`, `make`, and the required Qt development libraries installed on your system.

### Build Instructions

To build the project, run the following command in the root directory:

```bash
cmake . && make
```

### Run Instructions

Once compiled, execute the binary:

```bash
./ShamelaOpener
```

## 📦 System Dependencies

The application includes a built-in dependency checker for Arch Linux that verifies the presence of:
- `fontconfig`
- `freetype2`
- `glibc`
- `hicolor-icon-theme`
- `libselinux`
- `zlib`

## 📝 License
This project is licensed under the GPL-3.0 License. See the `LICENSE` file for details.
