all: font_8x16.c font_16x32.c

font_8x16.c: font_8x16.png preparefont
	./preparefont 32 8 16 $@ $<

font_16x32.c: font_16x32.png preparefont
	./preparefont 47 16 32 $@ $<

preparefont: preparefont.cpp png.h png.cpp
	g++ -o$@ preparefont.cpp png.cpp -lpng
