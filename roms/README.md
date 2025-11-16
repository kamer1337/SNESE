# ROM Directory

Place your SNES ROM files (.sfc or .smc) in this directory.

When you start SNESE without specifying a ROM file, the GUI will automatically
scan this directory and display a list of available ROMs for you to select.

## Supported File Types
- .sfc (Super Famicom)
- .smc (Super Nintendo)
- Both uppercase and lowercase extensions are supported

## Usage

### Method 1: ROM Selection GUI (New!)
```bash
./snesemu
```
This will show a menu with all available ROMs in this directory.

### Method 2: Direct ROM Loading
```bash
./snesemu roms/your-rom-file.sfc
```

### Method 3: Game Maker Mode
```bash
./snesemu --maker roms/your-rom-file.sfc
```

## Legal Notice

Only use ROM files that you legally own. The SNESE emulator is for educational
purposes only. Do not use this software to play pirated games.
