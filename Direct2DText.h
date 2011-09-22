
#include <d2d1.h>
#include <dwrite.h>

typedef
	lib::wr::unique_com_interface<ID2D1Factory>::type
unique_com_d2d1factory;

typedef
	lib::wr::unique_com_interface<ID2D1HwndRenderTarget>::type
unique_com_d2d1hwndrendertarget;

typedef
	lib::wr::unique_com_interface<ID2D1SolidColorBrush>::type
unique_com_d2d1solidcolorbrush;

typedef
	lib::wr::unique_com_interface<IDWriteFactory>::type
unique_com_d2d1dwritefactory;

typedef
	lib::wr::unique_com_interface<IDWriteTextFormat>::type
unique_com_d2d1dwritetextformat;

typedef
	lib::wr::unique_com_interface<IDWriteTextLayout>::type
unique_com_d2d1dwritetextlayout;

typedef
	lib::wr::unique_com_interface<IDWriteInlineObject>::type
unique_com_d2d1dwriteinlineobject;

typedef
	lib::wr::unique_com_interface<IDWriteTypography>::type
unique_com_d2d1dwritetypography;

