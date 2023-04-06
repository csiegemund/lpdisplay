SET BDFCONV=%USERPROFILE%\OneDrive\E-Projekte\Tools\bdfconv.exe
CD %USERPROFILE%\OneDrive\Dokumente\PlatformIO\Projects\TestPIO\fonts

%BDFCONV% -f 1 -m"0-126,128-134,160-255" -n audiofont_5x8 -o audiofont_5x8.c audiofont_5x8.bdf

%BDFCONV% -f 1 -m"0-126,128-134,160-255" -n audiofont_6x12 -o audiofont_6x12.c audiofont_6x12.bdf

%BDFCONV% -f 1 -m"0-36,128-134" -n audioicons_20x20 -o audioicons_20x20.c audioicons_20x20.bdf

COPY /B *.c ..\src\my_fonts.c 

PAUSE


