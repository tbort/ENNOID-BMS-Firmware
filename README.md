# Other parts of this project

This is the firmware repository containing all firmware source files. There are three more repositories for this project:

[ENNOID-BMS Hardware](https://github.com/EnnoidMe/ENNOID-BMS) The hardware sourcefiles.

[ENNOID-BMS Bootloader](https://github.com/EnnoidMe/DieBieMS-Bootloader) Same as DieBieMS bootloader. Can be flashed with the BMS Tool in the firmware tab or with an STlink-V2. 

[ENNOID-BMS Configuration tool](https://github.com/EnnoidMe/ENNOID-BMS-Tool) the tool to configure the ENNOID-BMS and upload the bootloader / update the main firmware.


# Building with GCC

- Install arm-none-eabi-gcc
- Configure `generalDefines.h` for LV, HV, SS, or SS_LITE versions
- Makefile has rules for flashing with STLink or JLink
- Could also load using bootloader with binary

When flashing the application the start address should be: <b>0x08000000</b>
When flashing the bootloader the start address should be: <b>0x08032000</b>

The flash is formatted as follows (summary):

((uint32_t)0x08000000) /* Base @ of Page 0, 2 Kbytes */  // Startup Code - Main application<br>
((uint32_t)0x08000800) /* Base @ of Page 1, 2 Kbytes */  // Page0 - EEPROM emulation<br>
((uint32_t)0x08001000) /* Base @ of Page 2, 2 Kbytes */  // Page1 - EEPROM emulation<br>
((uint32_t)0x08001800) /* Base @ of Page 3, 2 Kbytes */  // Remainder of the main application firmware stars from here.<br>
((uint32_t)0x08019000) /* Base @ of Page 50, 2 Kbytes */  // New app firmware base addres<br>
((uint32_t)0x08032000) /* Base @ of Page 100, 2 Kbytes */  // Bootloader base<br>

See "modFlash.h" and "modFlash.c" for more info.

# Install OpenOCD for Debugging with VSCode for Window
- Install VSCode with C/C++, C/C++ extension pack, and Cortex-Debug, Makefile Tools, Serial Monitor
- Download MSYS2 or Git Bash
- Download xPack OpenOCD 
- Download gcc-arm-none-eabi 
- Extract, and install to its corresponding folder (i.e. C:/openocd/... , C:/gcc-arm-none-eabi/... , )
- Install make with Git Bash/MSYS2
- Update path to arm tool chain : $ echo 'export PATH=$PATH:/c/gcc-arm-none-eabi/bin/' >> ~/.bashrc && source ~/.bashrc
- Go to ./build_all/rebuild_all to find corresponding "make" -> run make to create main.elf 
- On VSCode, click on debug icon on left side, and click on "create a launch.json file" ; choose Cortex_Debug
- Copy and change corresponding path for configFiles and searchDir
````
{
    "version": "0.2.0",
    "configurations": [
        {
            "cwd": "${workspaceRoot}",
            "executable": "./main.elf",
            "name": "Debug with OpenOCD",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "openocd",
            "configFiles": [
                "C:/openocd/scripts/interface//stlink.cfg",
                "C:/openocd/scripts/target/stm32f3x.cfg"
            ],
            "searchDir": [
                "C:/openocd/scripts"
            ],
            "runToEntryPoint": "main",
            "showDevDebugOutput": "none"
        },
    ]
}
````

- Create settings.json, copy and paste: 
````
{
    "cortex-debug.gdbPath": "C:/gcc-arm-none-eabi/bin/arm-none-eabi-gdb",
    "cortex-debug.openocdPath": "C:/openocd/bin/openocd",
    "cortex-debug.variableUseNaturalFormat": true
}
````

-Click on green button on left "Debug with OpenOCD"

Example video: https://www.youtube.com/watch?v=-p26X8lTAvo&ab_channel=EkaSusilo 