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

The code is hard configured to use an SSD1309 OLED display with 128 x 64 pixels. As it used the famous [u8g2 library](https://github.com/olikraus/u8g2/wiki) you could easily port the code to almost any 128x64 display on the market...
