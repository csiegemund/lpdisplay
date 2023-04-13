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
