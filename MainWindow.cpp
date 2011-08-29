
#include "stdafx.h"
#include "scanner.h"

#include "MainWindow.tmh"

namespace MainWindow
{
	struct tag {};

	struct D2D1CellPoint
	{
		size_t column;
		size_t row;
	};

	struct D2D1CellSize
	{
		size_t columns;
		size_t rows;
	};

	struct D2D1CellRect
	{
		D2D1CellPoint begin;
		D2D1CellPoint end;
	};

	struct D2D1CellLayout
	{
		D2D1CellSize min;
		D2D1CellSize desired;
		D2D1CellSize max;
	};

	struct D2D1Resources
	{
		D2D1Resources()
		{
			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			lib::unique_gdi_release_dc screen(std::make_pair(nullptr, GetDC(nullptr)));
			dpiScaleX = GetDeviceCaps(screen.get().second, LOGPIXELSX) / 96.0f;
			dpiScaleY = GetDeviceCaps(screen.get().second, LOGPIXELSY) / 96.0f;

			// Create Direct2D factory.
			hr.reset(D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				d2DFactory.replace()
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "D2D1CreateFactory: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			// Create a shared DirectWrite factory.
			hr.reset(DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(dWriteFactory.replace())
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "DWriteCreateFactory: %!HRESULT!", hr.get());
				hr.throw_if();
			}

		}

		// How much to scale a design that assumes 96-DPI pixels.
		float dpiScaleX;
		float dpiScaleY;

		// Direct2D
		unique_com_d2d1factory d2DFactory;

		// DirectWrite
		unique_com_d2d1dwritefactory dWriteFactory;
	};

	struct generationRange
	{
		size_t begin;
		size_t end;
	};

	template<typename DataTag>
	struct generationBroker
	{
	private:
		static size_t* generation() { static size_t current = 0; return &current; }

	public:
		static size_t current() {return InterlockedExchangeAdd(generation(), 0);}
		static size_t next() {return InterlockedExchangeAdd(generation(), 1);}
	};

	template<typename DataTag>
	struct snapshotOfData
	{
	private:
		typedef
			std::list<size_t>
		Snapshots;

		static Snapshots& snapshots() { static Snapshots result; return result; }
		static size_t* oldestSnapshot() { static size_t result = 0; return &result; }

		size_t generation;
		Snapshots::iterator location;

	public:
		~snapshotOfData()
		{
			if (snapshots().end() != location)
			{
				snapshots().erase(location);
			}
			if (!snapshots().empty())
			{
				InterlockedCompareExchange(oldestSnapshot(), snapshots().front(), *oldestSnapshot());
			}
		}

		snapshotOfData()
			: generation(std::numeric_limits<size_t>::max())
			, location(snapshots().end())
		{
		}

		snapshotOfData(snapshotOfData&& other)
			: generation(other.generation)
			, location(other.location)
		{
			other.location = snapshots().end();
		}

		snapshotOfData& operator=(snapshotOfData other)
		{
			using std::swap;
			swap(other.generation, generation);
			swap(other.location, location);

			return *this;
		}

		size_t get() const {return generation;}

		static size_t oldest()
		{
			return InterlockedExchangeAdd(oldestSnapshot(), 0);
		}

		static snapshotOfData createSnapshot()
		{
			snapshotOfData result;
			result.generation = generationBroker<DataTag>::current();
			result.location = snapshots().insert(snapshots().end(), result.generation);
			return std::move(result);
		}
	};


	template<typename T, typename DataTag>
	struct retainedData
	{
	private:
		struct Data
		{
			size_t validFrom;
			T data;
		};
		std::vector<Data> dataHistory;

	public:
		retainedData()
		{
			set(T());
		}

		explicit retainedData(T data)
		{
			set(std::move(data));
		}

		retainedData(retainedData&& other)
			: dataHistory(std::move(other.dataHistory))
		{
		}

		retainedData& operator=(retainedData other)
		{
			using std::swap;
			swap(dataHistory, other.dataHistory);
			return *this;
		}


		const T& get(const snapshotOfData<DataTag>& snapshot) const
		{
			auto found = std::find_if(dataHistory.begin(), dataHistory.end(), 
				[&] (decltype(dataHistory[0]) item) -> bool
				{
					if (item.validFrom <= snapshot.get())
					{
						return true;
					}
					return false;
				});
			if (found != dataHistory.end())
			{
				return found->data;
			}
			throw std::runtime_error("no data");
		}

		void set(T data)
		{
			Data entry = {};
			entry.validFrom = generationBroker<DataTag>::next();
			entry.data = data;
			dataHistory.emplace(dataHistory.begin(), std::move(entry));

			auto cursor = dataHistory.begin();
			auto end = dataHistory.end();
			for(;cursor != end; ++cursor)
			{
				if (cursor->validFrom < snapshotOfData<DataTag>::oldest())
				{
					dataHistory.erase(cursor, end);
					break;
				}
			}
		}
	};

	struct uiData {};

	namespace TextAlignment
	{
		enum type {
			Invalid = 0,

			Left,
			Center,
			Right
		};
	}
	namespace TextParagraph
	{
		enum type {
			Invalid = 0,

			Top,
			Center,
			Bottom
		};
	}
	struct TextStyle
	{
		TextAlignment::type alignment;
		TextParagraph::type paragraph;
		FLOAT fontSize;
		std::wstring fontName;
	};

	struct Label
	{
		retainedData<TextStyle, uiData> textStyle;
		retainedData<std::wstring, uiData> text;
	};

	DWRITE_TEXT_ALIGNMENT d2d1From(TextAlignment::type value)
	{
		switch (value)
		{
		case TextAlignment::Left: return DWRITE_TEXT_ALIGNMENT_LEADING;
		case TextAlignment::Right: return DWRITE_TEXT_ALIGNMENT_TRAILING;
		case TextAlignment::Center: return DWRITE_TEXT_ALIGNMENT_CENTER;
		};
		throw std::logic_error("invalid TextAlignment");
	}

	DWRITE_PARAGRAPH_ALIGNMENT d2d1From(TextParagraph::type value)
	{
		switch (value)
		{
		case TextParagraph::Top: return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		case TextParagraph::Bottom: return DWRITE_PARAGRAPH_ALIGNMENT_FAR;
		case TextParagraph::Center: return DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		};
		throw std::logic_error("invalid TextParagraph");
	}

	struct D2D1Label
	{
		unique_com_d2d1dwritetextformat textFormat;
		unique_com_d2d1dwritetextlayout textLayout;
		unique_com_d2d1solidcolorbrush blackBrush;

		D2D1CellLayout CalculateLayout(D2D1Resources& resources, unique_com_d2d1hwndrendertarget& hwndRenderTarget, const snapshotOfData<uiData>& snapshot, const Label& label, D2D1_SIZE_F cell, FLOAT gutter, const D2D1CellRect& available)
		{
			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			// Create a black brush.
			hr.reset(hwndRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(
					D2D1::ColorF::Black
					),
				blackBrush.replace()
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateSolidColorBrush: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			auto& style = label.textStyle.get(snapshot);

			hr.reset(resources.dWriteFactory->CreateTextFormat(
				style.fontName.c_str(),                 // Font family name.
				NULL,                        // Font collection (NULL sets it to use the system font collection).
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				style.fontSize,
				L"en-us",
				textFormat.replace()
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateTextFormat: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			// Center align (horizontally) the text.
			hr.reset(textFormat->SetTextAlignment(d2d1From(style.alignment)));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetTextAlignment: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			// Center align (vertically) the text.
			hr.reset(textFormat->SetParagraphAlignment(d2d1From(style.paragraph)));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetParagraphAlignment: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			FLOAT scaledFontSize = ((style.fontSize / 72.0f) * resources.dpiScaleY);

			auto& text = label.text.get(snapshot);
			FLOAT maxWidth = (cell.width + gutter) * (available.end.column - available.begin.column);
			size_t textRows = static_cast<size_t>(scaledFontSize / (cell.height + gutter));
			FLOAT textHeight = (cell.height + gutter) * textRows;

			hr.reset(resources.dWriteFactory->CreateTextLayout(
				text.data(),      // The string to be laid out and formatted.
				text.size(),  // The length of the string.
				textFormat.get(),  // The text format to apply to the string (contains font information, etc).
				maxWidth,         // The width of the layout box.
				textHeight,        // The height of the layout box.
				textLayout.replace()  // The IDWriteTextLayout interface pointer.
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateTextLayout: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			hr.reset(textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetWordWrapping: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			unique_com_d2d1dwriteinlineobject ellipsis;
			hr.reset(resources.dWriteFactory->CreateEllipsisTrimmingSign(textFormat.get(), ellipsis.replace()));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateEllipsisTrimmingSign: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			DWRITE_TRIMMING trim = {};
			trim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
			hr.reset(textLayout->SetTrimming(&trim, ellipsis.get()));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetTrimming: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			FLOAT minWidth = 0.0f;
			hr.reset(textLayout->DetermineMinWidth(&minWidth));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateTextLayout: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			D2D1CellLayout result;
			result.desired.columns = static_cast<size_t>(minWidth / (cell.width + gutter));
			result.desired.rows = textRows;
			result.min.columns = static_cast<size_t>(10 * (scaledFontSize * .62));
			result.min.rows = textRows;
			result.max.columns = result.desired.columns;
			result.max.rows = textRows;
			return result;
		}

		D2D1CellLayout QueryLayout(D2D1Resources& resources, unique_com_d2d1hwndrendertarget& hwndRenderTarget, const snapshotOfData<uiData>& snapshot, const Label& label, D2D1_SIZE_F cell, FLOAT gutter, const D2D1CellRect& available)
		{
			return CalculateLayout(resources, hwndRenderTarget, snapshot, label, cell, gutter, available);
		}

		void Layout(D2D1Resources& resources, unique_com_d2d1hwndrendertarget& hwndRenderTarget, const snapshotOfData<uiData>& snapshot, const Label& label, D2D1_SIZE_F cell, FLOAT gutter, const D2D1CellSize& size)
		{
			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			// Create a black brush.
			hr.reset(hwndRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(
					D2D1::ColorF::Black
					),
				blackBrush.replace()
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateSolidColorBrush: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			auto& style = label.textStyle.get(snapshot);

			hr.reset(resources.dWriteFactory->CreateTextFormat(
				style.fontName.c_str(),                 // Font family name.
				NULL,                        // Font collection (NULL sets it to use the system font collection).
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				style.fontSize,
				L"en-us",
				textFormat.replace()
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateTextFormat: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			// Center align (horizontally) the text.
			hr.reset(textFormat->SetTextAlignment(d2d1From(style.alignment)));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetTextAlignment: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			// Center align (vertically) the text.
			hr.reset(textFormat->SetParagraphAlignment(d2d1From(style.paragraph)));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetParagraphAlignment: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			auto& text = label.text.get(snapshot);

			hr.reset(resources.dWriteFactory->CreateTextLayout(
				text.data(),      // The string to be laid out and formatted.
				text.size(),  // The length of the string.
				textFormat.get(),  // The text format to apply to the string (contains font information, etc).
				(cell.width + gutter) * size.columns,         // The width of the layout box.
				(cell.height + gutter) * size.rows,        // The height of the layout box.
				textLayout.replace()  // The IDWriteTextLayout interface pointer.
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateTextLayout: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			hr.reset(textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetWordWrapping: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			unique_com_d2d1dwriteinlineobject ellipsis;
			hr.reset(resources.dWriteFactory->CreateEllipsisTrimmingSign(textFormat.get(), ellipsis.replace()));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateEllipsisTrimmingSign: %!HRESULT!", hr.get());
				hr.throw_if();
			}

			DWRITE_TRIMMING trim = {};
			trim.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
			hr.reset(textLayout->SetTrimming(&trim, ellipsis.get()));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetTrimming: %!HRESULT!", hr.get());
				hr.throw_if();
			}
		}

		void Draw(unique_com_d2d1hwndrendertarget& hwndRenderTarget, D2D1Resources& resources, D2D1_POINT_2F origin)
		{
			if (textLayout)
			{
				hwndRenderTarget->DrawTextLayout(
					origin,
					textLayout.get(),
					blackBrush.get()
					);
			}
			UNREFERENCED_PARAMETER(resources);
		}
	};

	struct type
	{
		type() 
			: window(nullptr)
			, instance(nullptr)
		{
			label.text.set(L"Hello World using   DirectWrite!");
			TextStyle style;
			style.alignment = TextAlignment::Left;
			style.paragraph = TextParagraph::Top;
			style.fontName = L"Gabriola";
			style.fontSize = 72.0f;
			label.textStyle.set(style);
		}

		Label label;

		D2D1Resources resources;
		D2D1Label d2d1Label;


		unique_hresult CreateDeviceIndependentResources()
		{
			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			// Create a text format using Gabriola with a font size of 72.
			// This sets the default font, weight, stretch, style, and locale.
			hr.reset(resources.dWriteFactory->CreateTextFormat(
				L"Gabriola",                 // Font family name.
				NULL,                        // Font collection (NULL sets it to use the system font collection).
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				72.0f,
				L"en-us",
				textFormat.replace()
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateTextFormat: %!HRESULT!", hr.get());
				return hr;
			}

			// Center align (horizontally) the text.
			hr.reset(textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetTextAlignment: %!HRESULT!", hr.get());
				return hr;
			}

			// Center align (vertically) the text.
			hr.reset(textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetParagraphAlignment: %!HRESULT!", hr.get());
				return hr;
			}

			return hr;
		}

		unique_hresult LayoutText(lib::range<PCWSTR> text)
		{
			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			// Create a text layout using the text format.
			RECT rect = {};
			GetClientRect(window, &rect); 
			float width  = rect.right  / dpiScaleX;
			float height = rect.bottom / dpiScaleY;
			hr.reset(resources.dWriteFactory->CreateTextLayout(
				text.begin(),      // The string to be laid out and formatted.
				text.size(),  // The length of the string.
				textFormat.get(),  // The text format to apply to the string (contains font information, etc).
				width,         // The width of the layout box.
				height,        // The height of the layout box.
				textLayout.replace()  // The IDWriteTextLayout interface pointer.
				));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateTextLayout: %!HRESULT!", hr.get());
				return hr;
			}

			{
				// Format the "DirectWrite" substring to be of font size 100.
				DWRITE_TEXT_RANGE textRange = {20,        // Start index where "DirectWrite" appears.
												6 };      // Length of the substring "Direct" in "DirectWrite".
				hr.reset(textLayout->SetFontSize(100.0f, textRange));
				if (!hr)
				{
					Trace(TRACE_LEVEL_ERROR, "SetFontSize: %!HRESULT!", hr.get());
					return hr;
				}
			}

			{
				// Format the word "DWrite" to be underlined.
				DWRITE_TEXT_RANGE textRange = {20,      // Start index where "DirectWrite" appears.
												11 };    // Length of the substring "DirectWrite".
				hr.reset(textLayout->SetUnderline(TRUE, textRange));
				if (!hr)
				{
					Trace(TRACE_LEVEL_ERROR, "SetUnderline: %!HRESULT!", hr.get());
					return hr;
				}
			}

			{
				// Format the word "DWrite" to be bold.
				DWRITE_TEXT_RANGE textRange = {20,
												11 };
				hr.reset(textLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, textRange));
				if (!hr)
				{
					Trace(TRACE_LEVEL_ERROR, "SetFontWeight: %!HRESULT!", hr.get());
					return hr;
				}
			}

			// Declare a typography pointer.
			unique_com_d2d1dwritetypography typography;

			// Create a typography interface object.
			hr.reset(resources.dWriteFactory->CreateTypography(typography.replace()));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "CreateTypography: %!HRESULT!", hr.get());
				return hr;
			}

			// Set the stylistic set.
			DWRITE_FONT_FEATURE fontFeature = {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_7,
											   1};
			hr.reset(typography->AddFontFeature(fontFeature));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "AddFontFeature: %!HRESULT!", hr.get());
				return hr;
			}

			// Set the typography for the entire string.
			DWRITE_TEXT_RANGE textRange = {0,
											text.size()};
			hr.reset(textLayout->SetTypography(typography.get(), textRange));
			if (!hr)
			{
				Trace(TRACE_LEVEL_ERROR, "SetTypography: %!HRESULT!", hr.get());
				return hr;
			}

			return hr;
		}

		unique_hresult CreateDeviceResources()
		{
			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			RECT rc = {};
			GetClientRect(
				window,
				&rc
				);

			D2D1_SIZE_U size = D2D1::SizeU(
				rc.right - rc.left,
				rc.bottom - rc.top
			);

			if (!hwndRenderTarget)
			{
				// Create a Direct2D render target.
				hr.reset(resources.d2DFactory->CreateHwndRenderTarget(
					D2D1::RenderTargetProperties(),
					D2D1::HwndRenderTargetProperties(
						window,
						size
						),
					hwndRenderTarget.replace()
					));
				if (!hr)
				{
					Trace(TRACE_LEVEL_ERROR, "CreateHwndRenderTarget: %!HRESULT!", hr.get());
					return hr;
				}

				// Create a black brush.
				hr.reset(hwndRenderTarget->CreateSolidColorBrush(
					D2D1::ColorF(
						D2D1::ColorF::Black
						),
					blackBrush.replace()
					));
				if (!hr)
				{
					Trace(TRACE_LEVEL_ERROR, "CreateSolidColorBrush: %!HRESULT!", hr.get());
					return hr;
				}
			}

			return hr;
		}

		void DiscardDeviceResources()
		{
			TRACE_SCOPE("void");
			hwndRenderTarget.reset();
			blackBrush.reset();
		}

		void DrawText()
		{
			TRACE_SCOPE("void");

			D2D1_POINT_2F origin = D2D1::Point2F(
				textBox.left,
				textBox.top
				);

			hwndRenderTarget->DrawTextLayout(
				origin,
				textLayout.get(),
				blackBrush.get()
				);
		}

		D2D1_SIZE_F size;
		size_t columns;
		size_t rows;
		FLOAT fontSize;
		FLOAT gutterSize;
		D2D1_SIZE_F cellSize;
		FLOAT edgeWidth;
		FLOAT edgeHeight;
		D2D1_RECT_F textBox;

		D2D1_RECT_F CalcRectF(size_t columnBegin, size_t rowBegin, size_t columnEnd, size_t rowEnd)
		{
			return D2D1::RectF( 
				edgeWidth + gutterSize + (columnBegin * cellSize.width) + (columnBegin * gutterSize),
				edgeHeight + gutterSize + (rowBegin * cellSize.height) + (rowBegin * gutterSize),
				edgeWidth + (columnEnd * cellSize.width) + (columnEnd * gutterSize),
				edgeHeight + (rowEnd * cellSize.height) + (rowEnd * gutterSize)
			);
		}

		unique_hresult DrawD2DContent()
		{
			unique_hresult hr;

			hr = CreateDeviceResources();
			if (!hr)
			{
				return hr;
			}

			hwndRenderTarget->BeginDraw();
			ON_UNWIND_AUTO([&]{
				if (!hr)
				{
					DiscardDeviceResources();
				}
				hwndRenderTarget->EndDraw();
			});

			hwndRenderTarget->SetTransform(D2D1::IdentityMatrix());

			hwndRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

			// Call the DrawText method of this class.
			//DrawText();
			d2d1Label.Draw(hwndRenderTarget, resources, D2D1::Point2F(edgeWidth + gutterSize, edgeHeight + gutterSize));

			size_t cursorRow = 0;
			size_t endRow = rows;
			for (;cursorRow != endRow; ++cursorRow)
			{
				size_t cursorColumn = 0;
				size_t endColumn = columns;
				for (;cursorColumn != endColumn; ++cursorColumn)
				{
					auto cell = CalcRectF(cursorColumn, cursorRow, cursorColumn + 1, cursorRow + 1);

					hwndRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(cell, 2.0, 2.0), blackBrush.get());
				}
			}

			return hr;
		}

		void Layout()
		{
			FLOAT phi = 1.62f;
			FLOAT Phi = 0.62f;
			FLOAT inch = 96.0f;

			fontSize = ((12.0f / 72.0f) * inch); // <font size in points> / <points in  an inch>
			gutterSize = (fontSize * Phi);

			cellSize = D2D1::SizeF(
				10 * fontSize * phi,
				fontSize
			);

			columns = static_cast<size_t>((size.width - gutterSize) / (cellSize.width + gutterSize));
			rows = static_cast<size_t>((size.height - gutterSize) / (cellSize.height + gutterSize)); 

			edgeWidth = (size.width - ((cellSize.width * columns) + (gutterSize * (columns + 1)))) / 2;
			edgeHeight = (size.height - ((cellSize.height * rows) + (gutterSize * (rows + 1)))) / 2;

			auto snapshot = snapshotOfData<uiData>::createSnapshot();

			D2D1CellSize layout = {};
			layout.columns = 2;
			layout.rows = 2;
			if (hwndRenderTarget)
			{
				d2d1Label.Layout(resources, hwndRenderTarget, snapshot, label, cellSize, gutterSize, layout);
			}
		}

		unique_com_xmlreader xmlreader;


		LRESULT OnNCCreate(const lib::Context<type>* context, HWND hwnd, LPCREATESTRUCT createStruct)
		{
			unique_hresult hr;

			TRACE_SCOPE("%!HRESULT!", hr.get());

			FAIL_FAST_IF(!hr, hr.get());

			window = hwnd;
			instance = createStruct->hInstance;

			lib::unique_gdi_release_dc screen(std::make_pair(nullptr, GetDC(nullptr)));
			dpiScaleX = GetDeviceCaps(screen.get().second, LOGPIXELSX) / 96.0f;
			dpiScaleY = GetDeviceCaps(screen.get().second, LOGPIXELSY) / 96.0f;

			hr.reset(CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void**>(xmlreader.replace()), nullptr));
			if (!hr)
			{
				return FALSE;
			}

			unique_com_stream stream;
			hr.reset(FileStream::OpenFile(
				L"main.xml",
				stream.replace()
			));
			if (!hr)
			{
				return FALSE;
			}

			hr.reset(xmlreader->SetInput(stream.get()));
			if (!hr)
			{
				return FALSE;
			}

			XmlNodeType nodeType = XmlNodeType_None; 
			while (hr) 
			{
				hr.reset(xmlreader->Read(&nodeType));
				if (hr != unique_hresult::cast(S_OK))
				{
					break;
				}
				hr.suppress();

				if (nodeType == XmlNodeType_Element)
				{
					LPCWSTR localName = nullptr;
					UINT localSize = 0;

					hr.reset(xmlreader->GetLocalName(&localName, &localSize));
					if (!hr)
					{
						return FALSE;
					}
					auto localRange = lib::make_range(localName, localName + localSize);
					if (lib::as_literal(L"Component") == localRange)
					{
					}
				}
			}
			if (!hr)
			{
				return FALSE;
			}

			hr = CreateDeviceIndependentResources();
			if (!hr)
			{
				return FALSE;
			}

			RECT rc = {};
			GetClientRect(
				window,
				&rc
				);

			size = D2D1::SizeF(
				static_cast<FLOAT>((rc.right - rc.left) / dpiScaleX),
				static_cast<FLOAT>((rc.bottom - rc.top) / dpiScaleY)
			);

			Layout();

			hr = LayoutText(lib::make_range_raw(L"Hello World using   DirectWrite!"));
			if (!hr)
			{
				return FALSE;
			}

			std::tie(hr, wiadevmgr2) = lib::ComCreateInstance<IWiaDevMgr2>(CLSID_WiaDevMgr2);
			if (!hr)
			{
				return FALSE;
			}

			unique_com_wiaenumdevinfo wiaenumdevinfo;

			if (!unique_hresult::make(wiadevmgr2->EnumDeviceInfo( WIA_DEVINFO_ENUM_LOCAL, wiaenumdevinfo.replace() )))
			{
				return FALSE;
			}

			unique_com_wiapropertystorage wiapropertystorage;

			ULONG count = 0;
			while (unique_hresult::make(wiaenumdevinfo->Next( 1, wiapropertystorage.replace(), NULL )).suppress() == unique_hresult::cast(S_OK))
			{
				wiapropertystorage->GetCount(&count);
			}

			return TRUE;
			UNREFERENCED_PARAMETER(context);
		}

		LRESULT OnCommand(const lib::Context<type>* context, HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
		{
			TRACE_SCOPE("id: %d", id);

			// Parse the menu selections:
			switch (id)
			{
			case IDM_ABOUT:
				DialogBox(instance, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hwnd);
				break;
			default:
				return DefWindowProc(context->window, context->message, context->wParam, context->lParam);
			}

			return 0;
			UNREFERENCED_PARAMETER(codeNotify);
			UNREFERENCED_PARAMETER(hwndCtl);
		}

		LRESULT OnSize(const lib::Context<type>* context, HWND hwnd, UINT state, int cx, int cy)
		{
			size = D2D1::SizeF(
				static_cast<FLOAT>(cx / dpiScaleX),
				static_cast<FLOAT>(cy / dpiScaleY)
			);

			Layout();

			if (textLayout)
			{
				textBox = CalcRectF(
					1, 
					1, 
					columns - 1, 
					2);

				auto textSize = D2D1::SizeF(
					textBox.right - textBox.left,
					textBox.bottom - textBox.top
				);

				textLayout->SetMaxWidth(textSize.width);
				textLayout->SetMaxHeight(textSize.height);
			}

			if (hwndRenderTarget)
			{
				D2D1_SIZE_U size;
				size.width = cx;
				size.height = cy;
				hwndRenderTarget->Resize(size);
			}

			return 0;
			UNREFERENCED_PARAMETER(hwnd);
			UNREFERENCED_PARAMETER(context);
			UNREFERENCED_PARAMETER(state);
		}

		LRESULT OnPaint(const lib::Context<type>* context, HWND hwnd)
		{
			LRESULT lresult = 0;
			TRACE_SCOPE("lresult: %d", lresult);

			PAINTSTRUCT ps = {};
			HDC hdc = NULL;

			hdc = BeginPaint(hwnd, &ps);
			lib::unique_gdi_end_paint ender(std::make_pair(hwnd, &ps));

			DrawD2DContent().suppress();

			return lresult;
			UNREFERENCED_PARAMETER(context);
		}

		LRESULT OnDestroy(const lib::Context<type>* context, HWND hwnd)
		{
			LRESULT lresult = 0;
			TRACE_SCOPE("lresult: %d", lresult);

			PostQuitMessage(0);
			return lresult;
			UNREFERENCED_PARAMETER(hwnd);
			UNREFERENCED_PARAMETER(context);
		}

		HWND window;
		HINSTANCE instance;

		// How much to scale a design that assumes 96-DPI pixels.
		float dpiScaleX;
		float dpiScaleY;

		// Direct2D
		unique_com_d2d1hwndrendertarget hwndRenderTarget;
		unique_com_d2d1solidcolorbrush blackBrush;

		// DirectWrite
		unique_com_d2d1dwritetextformat textFormat;
		unique_com_d2d1dwritetextlayout textLayout;

		unique_com_wiadevmgr2 wiadevmgr2;
	};

	lib::window_class_traits_builder<type> window_class_traits(tag&&);

	void window_class_register(PCWSTR windowClass, WNDCLASSEX* wcex, tag&&)
	{
		wcex->hIcon = LoadIcon(wcex->hInstance, MAKEINTRESOURCE(IDI_SCANNER));
		wcex->hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex->lpszMenuName = MAKEINTRESOURCE(IDC_SCANNER);
		wcex->lpszClassName = windowClass;
		wcex->hIconSm = LoadIcon(wcex->hInstance, MAKEINTRESOURCE(IDI_SMALL));
	}

	typedef
		lib::window_class<tag>
	registrar;
}

//
//   FUNCTION: CreateMainWindow(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
std::pair<unique_winerror, lib::unique_close_window> 
CreateMainWindow(HINSTANCE hInstance, PCWSTR windowClass, PCWSTR title, int nCmdShow)
{
	unique_winerror winerror;
	TRACE_SCOPE("%!WINERROR!", winerror.get());

	MainWindow::registrar::Register(windowClass);

	lib::unique_close_window window;

	std::tie(winerror, window) = lib::winerror_and_close_window(
		CreateWindow(
			windowClass, 
			title, 
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 
			0, 
			CW_USEDEFAULT, 
			0, 
			NULL, 
			NULL, 
			hInstance, 
			NULL));

	if (!!window)
	{
		ShowWindow(window.get(), nCmdShow);
		UpdateWindow(window.get());
	}

	return std::make_pair(std::move(winerror), std::move(window));
}
