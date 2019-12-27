#if       _WIN32_WINNT < 0x0500
#undef  _WIN32_WINNT
#define _WIN32_WINNT   0x0500
#endif

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <ctype.h>


enum COLORS {
	RED = 1, LIME = 2, BLUE = 3, BROWN = 4, YELLOW = 5, GREEN = 6, CYAN = 7, SKYBLUE = 8,
	MAGENTA = 9, ORANGE = 10, WHITE = 11, BLACK = 100
};

class CC212VSGL
{
private:
	static HWND console_handle;
	static HDC device_context;
	static HPEN pen;

	static HDC mdc;
	static HBITMAP mbmp;
	static HBITMAP moldbmp;

	static int penSize;
	static int fontSize;
	static int fontBoldness;
	static COLORREF penColor;


	static COLORREF convertFromColors(COLORS c);
	static HFONT hFont;
	static LOGFONT logfont;
	static HFONT hNewFont;
	static HFONT hOldFont;




public:
	static void setup();
	static void drawLine(int xs, int ys, int xe, int ye);
	static COLORREF generateFromRGB(int r, int g, int b);
	static void setDrawingColor(COLORS c);
	static void setDrawingColor(COLORREF c);

	static void setDrawingThickness(int size);
	static void setFontSizeAndBoldness(int size, int boldness);
	static void resetFontSize();
	static void beginDraw();
	static void endDraw();
	static void drawCircle(int xc, int yc, int rc);
	static void drawEllipse(int xc, int yc, int xr, int yr);
	static void drawSolidEllipse(int xc, int yc, int xr, int yr);
	static void drawSolidCircle(int xc, int yc, int rc);
	static void drawRectangle(int x, int y, int width, int height);
	static void drawSolidRectangle(int x, int y, int width, int height);
	static void drawPixel(int x, int y);
	static bool drawImage(int imgID, int x, int y, COLORREF transparentKey);
	static int loadImage(const char* path);
	static void resizeImage(int imgID, int w, int h);
	static int deleteImage(int imgId);
	static int getWindowWidth();
	static int getWindowHeight();
	static void fillScreen(COLORS c);


	static void drawText(int x, int y, const char* text);
	static 	void moveCursor(int x, int y);

	static void hideCursor();
	static void showCursor();

	static void setFullScreenMode();

	static void setWindowScreenMode();
};



