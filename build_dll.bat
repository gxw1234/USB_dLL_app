@echo off
echo Building USB Application DLL...

:: Set compile environment
set CC=gcc
set DLL_NAME=USB_G2X.dll

:: Compile DLL
echo Compiling DLL...
%CC% -shared -o %DLL_NAME% usb_application.c usb_middleware.c usb_device.c usb_protocol.c usb_log.c usb_spi.c usb_bootloader.c  usb_power.c usb_gpio.c -DUSB_API_EXPORTS -I. -lsetupapi

:: Check compilation result
if %errorlevel% neq 0 (
    echo Error: Compilation failed
    exit /b 1
)

echo Compilation completed! Successfully generated %DLL_NAME%
exit /b 0
