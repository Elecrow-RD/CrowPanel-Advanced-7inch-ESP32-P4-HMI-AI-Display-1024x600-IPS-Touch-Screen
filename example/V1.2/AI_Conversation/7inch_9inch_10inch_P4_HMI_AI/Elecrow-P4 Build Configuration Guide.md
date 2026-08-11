# Elecrow P4 Development Board Build Configuration Guide

## 📋 How to Ensure the elecrow-p4-board Configuration Is Used During Build

### Method 1: Select the Board via menuconfig (Recommended)

This is the standard approach—choose the development board through the graphical configuration menu:

```bash
# 1. Set the build target to ESP32-P4
idf.py set-target esp32p4

# 2. Open the configuration menu
idf.py menuconfig

# 3. Navigate in the menu:
#    Xiaozhi Assistant → Board Type → Elecrow P4 Board
#    Press Space to select, then save and exit

# 4. Build
idf.py build

```

**Menu path**
```
Xiaozhi Assistant
  └─ Board Type
      └─ [*] Elecrow P4 Board  ← Select this
```

### Method 2: Use the release.py Script (Easiest)

This project-provided automation script configures the board for you:

```bash
python scripts/release.py elecrow-p4-board
```

This script will:
- Automatically set the target chip to ESP32-P4
- Automatically configure the development board type
- Automatically build and package the firmware

### Method 3: Manually Edit sdkconfig (Advanced)

If you're comfortable with configuration files, you can edit `sdkconfig` directly:

```bash
# 1. Set the build target
idf.py set-target esp32p4

# 2. Edit the sdkconfig file, add or modify:
CONFIG_BOARD_TYPE_ELECROW_P4_BOARD=y

# 3. Build
idf.py build
```

## 🔍 How to Verify the Correct Board Configuration Is Being Used

### Method 1: Check Build Output

During compilation, you will see output like this:

```
-- Found BOARD_TYPE: elecrow-p4-board
-- Found BOARD_NAME: elecrow-p4-board
```

### Method 2: Check Build Commands

During compilation, check the macro definitions in the build commands:

```bash
idf.py build -v  # Verbose output mode
```

You should see:
```
-DBOARD_TYPE="elecrow-p4-board"
-DBOARD_NAME="elecrow-p4-board"
```

### Method 3: Check Compiled Files

After compilation, check if the `build/main/` directory contains your board files:

```bash
# You should see these files being compiled:
# - boards/elecrow-p4-board/elecrow_board.cc
# - boards/elecrow-p4-board/config.h (included via macro definition)
```

### Method 4: Check sdkconfig File

After compilation, check the `sdkconfig` file:

```bash
grep BOARD_TYPE sdkconfig
```

You should see:
```
CONFIG_BOARD_TYPE_ELECROW_P4_BOARD=y
```

## ⚠️ Important Notes

### 1. Confirm Board Type Before Each Build

If you have previously compiled for another board, the `sdkconfig` may still contain the previous configuration. It is recommended to:

```bash
# Clean previous configuration
idf.py fullclean

# Reset the target
idf.py set-target esp32p4

# Reconfigure the board
idf.py menuconfig
```

### 2. Ensure Directory Name Is Correct

The board directory name must match the `name` field in `config.json`:
- Directory name: `main/boards/elecrow-p4-board/`
- Name in config.json: `"elecrow-p4-board"`

### 3. Ensure Files Exist

Before compilation, confirm the following files exist:
- ✅ `main/boards/elecrow-p4-board/config.h`
- ✅ `main/boards/elecrow-p4-board/elecrow_board.cc`
- ✅ `main/boards/elecrow-p4-board/config.json`

## 🚀 Quick Start

### First Build

```bash
# 1. Clean (can be skipped for the first time)
idf.py fullclean

# 2. Set target chip
idf.py set-target esp32p4

# 3. Configure the board
idf.py menuconfig
# Select: Xiaozhi Assistant → Board Type → Elecrow P4 Board

# 4. Build
idf.py build

# 5. Flash (after connecting the device)
idf.py flash monitor
```

### Subsequent Builds

If already configured, you can build directly:

```bash
idf.py build
```

## 📝 Configuration File Description

### sdkconfig File

After compilation, the `sdkconfig` file will be generated in the project root directory, containing:

```ini
# Board type configuration
CONFIG_BOARD_TYPE_ELECROW_P4_BOARD=y

# Other configurations...
CONFIG_IDF_TARGET="esp32p4"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
```

### build/compile_commands.json

After compilation, `build/compile_commands.json` will be generated. You can check the compilation commands to confirm the correct macro definitions are being used.

## 🔧 Common Issues

### Q1: Build Error - Board Files Not Found?

**A:** Check:
1. Is the directory name correct: `main/boards/elecrow-p4-board/`
2. Is the file name correct: `elecrow_board.cc`
3. Is the correct board selected in menuconfig

### Q2: Build Uses Configuration from Another Board?

**A:** 
1. Run `idf.py fullclean` to clean
2. Run `idf.py menuconfig` again to select the board
3. Confirm that `sdkconfig` only contains `CONFIG_BOARD_TYPE_ELECROW_P4_BOARD=y`

### Q3: How to Confirm the Currently Configured Board?

**A:** 
```bash
# Check sdkconfig
grep BOARD_TYPE sdkconfig

# Or check build output
idf.py build 2>&1 | grep BOARD_TYPE
```

## ✅ Checklist

Before compilation, confirm:

- [ ] `idf.py set-target esp32p4` has been run
- [ ] "Elecrow P4 Board" has been selected in menuconfig
- [ ] `sdkconfig` contains `CONFIG_BOARD_TYPE_ELECROW_P4_BOARD=y`
- [ ] All configuration files exist (config.h, elecrow_board.cc, config.json)
- [ ] Flash size is correctly configured (currently 16MB)

---

**Now you can safely compile! The elecrow-p4-board configuration will be used automatically during compilation.**
