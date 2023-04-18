# lpdisplay

Reads information from linkplays UART interface and visualize it on an OLED display

This could currently be used for Arylics DIY audio boards which provide access to the linkplay UART interface:
- Up2Stream Pro V3 - Multiroom Wireless Receiver Board
- Up2Stream Amp - Multiroom Wireless Streaming Stereo Amplifier Board

You'll need a microcontroller with hardware UART, hardware SPI or hardware I2C interface and 32K flash, eg:
- ATTINY 3216
- ATTINY 3217
- ATTINY 3226
- ATTINY 3227
- ARDUINO NANO EVERY

In platformio_overide.ini you can configure your used display + serial debug behaviour (if your controller supports more than one UART)

### Flashing and starting up with Visual Studio Code
* Install [Visual Studio Code](https://code.visualstudio.com/download) (from now named "vscode")
* In Visual Studio Code, install the [PlatformIO Extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
* Install git and enable git in vscode - [git download](https://git-scm.com/downloads/) - [Instructions](https://www.jcchouinard.com/install-git-in-vscode/)
* Clone this repository (you really have to clone it, don't just download the ZIP file. During the build process the git hash gets embedded into the firmware. If you download the ZIP file a build error will occur): Inside vscode open the command palette by pressing `CTRL` + `SHIFT` + `P`. Enter `git clone`, add the repository-URL `https://github.com/csiegemund/lpdisplay`. Next you have to choose (or create) a target directory.
* In vscode, choose File --> Open Folder and select the previously downloaded source code. (You have to select the folder which contains the "platformio.ini" and "platformio_override.ini" file)
* If you use an ATTINY32xx board you need to manually adjust the upload_port (COM port) in the file "platformio.ini" for your USB-to-updi-converter. 
* The upload_port of a Arduino board is searched automatically
* Select the arrow button in the blue bottom status bar (PlatformIO: Upload) to compile and upload the firmware. During the compilation, all required libraries are downloaded automatically.
* Under Linux, if the upload fails with error messages "Could not open /dev/ttyUSB0, the port doesn't exist", you can check via ```ls -la /dev/tty*``` to which group your port belongs to, and then add your user this group via ```sudo adduser <yourusername> dialout``` (if you are using ```arch-linux``` use: ```sudo gpasswd -a <yourusername> uucp```, this method requires a logout/login of the affected user).

### Wiring (Arduino Nano Every with I2C display)
<img src="https://github.com/csiegemund/lpdisplay/blob/master/docs/Wiring%20I2C_Steckplatine.svg" alt="Arduino Nano Every with I2C display" width="300" height="200">
