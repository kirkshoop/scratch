
#include "stdafx.h"
#include "scanner.h"

#include "MainWindow.tmh"

namespace UnknownObject
{
	struct tag {};

	struct type
		: public lib::com::refcount
	{
	};

	lib::ifset::traits_builder<typename lib::tv::factory<IUnknown>::type, type>
	interface_set_traits(tag&&);

	type* interface_storage(type* storage, lib::ifset::interface_tag<IUnknown>&&, tag&&)
	{
		return storage;
	}

	typedef
		lib::ifset::interface_set<tag>
	object;
}

namespace MainWindow
{
	struct tag {};

	struct D2D1RowRange
	{
		size_t begin;
		size_t end;

		inline size_t size() const { return end - begin;}
	};

	struct D2D1VerticalLine
	{
		size_t column;
		D2D1RowRange rows;
	};

	struct D2D1ColumnRange
	{
		size_t begin;
		size_t end;

		inline size_t size() const { return end - begin;}
	};

	struct D2D1HorizontalLine
	{
		size_t row;
		D2D1ColumnRange columns;
	};

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

	struct D2D1CellBox
	{
		D2D1ColumnRange columns;
		D2D1RowRange rows;
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

#if 1
			auto unknownObject = lib::com::interface_cast<IUnknown>(new UnknownObject::object());
			lib::wr::unique_com_unknown unknown;
			unknownObject->QueryInterface(unknown.replace());

			{
				auto fileStream = lib::com::interface_cast<IStream>(new com::file::object(L"C:\\save\\source\\main.cpp"));
				fileStream->Revert();
			}
#endif

			lib::wr::unique_gdi_release_dc screen(std::make_pair(nullptr, GetDC(nullptr)));
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
			entry.data = std::move(data);
			// TODO: insert needs to be made atomic
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

		bool set_if(const snapshotOfData<DataTag>& snapshot, T data)
		{
			// TODO: test-insert needs to be made atomic
			if (generationBroker<DataTag>::current() <= snapshot.get())
			{
				return false;
			}
			Data entry = {};
			entry.validFrom = generationBroker<DataTag>::next();
			entry.data = std::move(data);
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
			return true;
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
		bool wrap;
	};

	struct Label
	{
		retainedData<TextStyle, uiData> textStyle;
		retainedData<std::wstring, uiData> text;
	};

	struct VerticalSeparator
	{
		retainedData<std::wstring, uiData> leftComponent;
		retainedData<FLOAT, uiData> percentFromLeft;
	};


	struct HorizontalSeparator
	{
		retainedData<std::wstring, uiData> topComponent;
		retainedData<FLOAT, uiData> percentFromTop;
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

		D2D1CellLayout QueryLayout(D2D1Resources& resources, unique_com_d2d1hwndrendertarget& hwndRenderTarget, const snapshotOfData<uiData>& snapshot, const Label& label, D2D1_SIZE_F cell, FLOAT gutter, const D2D1CellRect& available)
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

			if (!style.wrap)
			{
				hr.reset(textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));
				if (!hr)
				{
					Trace(TRACE_LEVEL_ERROR, "SetWordWrapping: %!HRESULT!", hr.get());
					hr.throw_if();
				}
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

			if (!style.wrap)
			{
				hr.reset(textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));
				if (!hr)
				{
					Trace(TRACE_LEVEL_ERROR, "SetWordWrapping: %!HRESULT!", hr.get());
					hr.throw_if();
				}
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

	struct D2D1VerticalSeparator
	{
		unique_com_d2d1solidcolorbrush blackBrush;
		D2D1_SIZE_F layout;

		D2D1VerticalLine QueryLayout(
			D2D1Resources& resources, 
			unique_com_d2d1hwndrendertarget& hwndRenderTarget, 
			const snapshotOfData<uiData>& snapshot, 
			const VerticalSeparator& separator, 
			D2D1_SIZE_F cell, 
			FLOAT gutter, 
			const D2D1CellBox& available)
		{
			UNREFERENCED_PARAMETER(resources);
			UNREFERENCED_PARAMETER(cell);
			UNREFERENCED_PARAMETER(gutter);

			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			// Create a black brush.
			if (!blackBrush)
			{
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
			}

			auto& percentFromLeft = separator.percentFromLeft.get(snapshot);

			D2D1VerticalLine result;
			result.column = static_cast<size_t>(available.columns.begin + (percentFromLeft * available.columns.size()));
			result.rows.begin = available.rows.begin;
			result.rows.end = available.rows.end;
			return result;
		}

		void Layout(
			D2D1Resources& resources, 
			unique_com_d2d1hwndrendertarget& hwndRenderTarget, 
			const snapshotOfData<uiData>& snapshot, 
			const VerticalSeparator& separator, 
			D2D1_SIZE_F cell, 
			FLOAT gutter, 
			const D2D1CellBox& available)
		{
			UNREFERENCED_PARAMETER(resources);
			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			// Create a black brush.
			if (!blackBrush)
			{
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
			}

			auto& percentFromLeft = separator.percentFromLeft.get(snapshot);

			D2D1VerticalLine result;
			result.column = static_cast<size_t>(available.columns.begin + (percentFromLeft * available.columns.size()));
			result.rows.begin = available.rows.begin;
			result.rows.end = available.rows.end;

			layout.width = gutter;
			layout.height = (available.rows.end - available.rows.begin) * (cell.height + gutter);
		}

		void Draw(unique_com_d2d1hwndrendertarget& hwndRenderTarget, D2D1Resources& resources, D2D1_POINT_2F origin)
		{
			auto separator = D2D1::RectF(origin.x, origin.y, origin.x + layout.width, origin.y + layout.height);
			hwndRenderTarget->DrawRectangle(
				separator,
				blackBrush.get()
				);
			UNREFERENCED_PARAMETER(resources);
		}
	};

	struct D2D1HorizontalSeparator
	{
		unique_com_d2d1solidcolorbrush blackBrush;
		D2D1_SIZE_F layout;

		D2D1HorizontalLine QueryLayout(
			D2D1Resources& resources, 
			unique_com_d2d1hwndrendertarget& hwndRenderTarget, 
			const snapshotOfData<uiData>& snapshot, 
			const HorizontalSeparator& separator, 
			D2D1_SIZE_F cell, 
			FLOAT gutter, 
			const D2D1CellBox& available)
		{
			UNREFERENCED_PARAMETER(resources);
			UNREFERENCED_PARAMETER(cell);
			UNREFERENCED_PARAMETER(gutter);

			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			// Create a black brush.
			if (!blackBrush)
			{
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
			}

			auto& percentFromTop = separator.percentFromTop.get(snapshot);

			D2D1HorizontalLine result;
			result.row = static_cast<size_t>(available.rows.begin + (percentFromTop * available.rows.size()));
			result.columns.begin = available.columns.begin;
			result.columns.end = available.columns.end;
			return result;
		}

		void Layout(
			D2D1Resources& resources, 
			unique_com_d2d1hwndrendertarget& hwndRenderTarget, 
			const snapshotOfData<uiData>& snapshot, 
			const HorizontalSeparator& separator, 
			D2D1_SIZE_F cell, 
			FLOAT gutter, 
			const D2D1CellBox& available)
		{
			UNREFERENCED_PARAMETER(resources);
			unique_hresult hr;
			TRACE_SCOPE("%!HRESULT!", hr.get());

			// Create a black brush.
			if (!blackBrush)
			{
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
			}

			auto& percentFromTop = separator.percentFromTop.get(snapshot);

			D2D1HorizontalLine result;
			result.row = static_cast<size_t>(available.rows.begin + (percentFromTop * available.rows.size()));
			result.columns.begin = available.columns.begin;
			result.columns.end = available.columns.end;

			layout.height = gutter;
			layout.width = (available.columns.end - available.columns.begin) * (cell.width + gutter);
		}

		void Draw(unique_com_d2d1hwndrendertarget& hwndRenderTarget, D2D1Resources& resources, D2D1_POINT_2F origin)
		{
			auto separator = D2D1::RectF(origin.x, origin.y, origin.x + layout.width, origin.y + layout.height);
			hwndRenderTarget->DrawRectangle(
				separator,
				blackBrush.get()
				);
			UNREFERENCED_PARAMETER(resources);
		}
	};

	struct Component
	{
		typedef
			lib::of::one_of<lib::tv::factory<Label>::type>
		Item;

		typedef
			lib::of::one_of<
				lib::tv::factory<
					std::vector<VerticalSeparator>, 
					std::vector<HorizontalSeparator>, 
					std::vector<Item>
				>::type
			>
		Content;

		retainedData<std::vector<Content>, uiData> contents;
	};

	struct Data
	{
		typedef
			std::unordered_map<std::wstring, std::shared_ptr<Component>>
		Components;

		retainedData<Components, uiData> components; 
	};

	struct D2D1View
	{
		typedef
			lib::of::one_of<lib::tv::factory<D2D1Label>::type>
		D2D1Item;

		typedef
			lib::of::one_of<
				lib::tv::factory<
					std::vector<D2D1VerticalSeparator>, 
					std::vector<D2D1HorizontalSeparator>, 
					std::vector<D2D1Item>
				>::type
			>
		D2D1Content;

		typedef
			std::unordered_map<std::wstring, std::vector<D2D1Content>>
		D2D1Components;

		D2D1Components components; 
	};

	struct type
	{
		struct Called
		{
			template<typename T>
			void operator()(T&& )
			{
				MessageBeep(0);
			}
		};

		type() 
			: window(nullptr)
			, instance(nullptr)
		{
			label.text.set(L"Hello World using   DirectWrite!");
			TextStyle style;
			style.alignment = TextAlignment::Left;
			style.paragraph = TextParagraph::Top;
			style.fontName = L"Gabriola";
			style.fontSize = 24.0f;
			label.textStyle.set(style);

			deviceList.percentFromLeft.set(.33f);

			one_string.reset_at<0>(L"hi");
			one_string.call_if_else<0>([&](std::wstring&) { MessageBeep(0); }, [&]{});

			one_string.reset("low");

			Called called;
			one_string.call(called);
		}

		lib::of::one_of<lib::tv::factory<std::wstring, std::string>::type> one_string; 

		Data data;
		D2D1View view;

		Label label;
		VerticalSeparator deviceList;

		D2D1Resources resources;
		D2D1Label d2d1Label;

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
			d2d1Label.Draw(
				hwndRenderTarget, 
				resources, 
				D2D1::Point2F(
					edgeWidth + gutterSize, 
					edgeHeight + gutterSize
				)
			);

			size_t cursorRow = 0;
			size_t endRow = rows;
			for (;cursorRow != endRow; ++cursorRow)
			{
				size_t cursorColumn = 0;
				size_t endColumn = columns;
				for (;cursorColumn != endColumn; ++cursorColumn)
				{
					//auto cell = CalcRectF(cursorColumn, cursorRow, cursorColumn + 1, cursorRow + 1);

					//hwndRenderTarget->DrawRoundedRectangle(D2D1::RoundedRect(cell, 2.0, 2.0), blackBrush.get());
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
			layout.columns = columns;
			layout.rows = rows / 2;
			if (hwndRenderTarget)
			{
				d2d1Label.Layout(resources, hwndRenderTarget, snapshot, label, cellSize, gutterSize, layout);
			}
		}

		unique_com_xmlreader xmlreader;


		LRESULT OnNCCreate(const lib::wnd::Context<type>* context, HWND hwnd, LPCREATESTRUCT createStruct)
		{
			unique_hresult hr;

			TRACE_SCOPE("%!HRESULT!", hr.get());

			FAIL_FAST_IF(!hr, hr.get());

			window = hwnd;
			instance = createStruct->hInstance;

			lib::wr::unique_gdi_release_dc screen(std::make_pair(nullptr, GetDC(nullptr)));
			dpiScaleX = GetDeviceCaps(screen.get().second, LOGPIXELSX) / 96.0f;
			dpiScaleY = GetDeviceCaps(screen.get().second, LOGPIXELSY) / 96.0f;


			RECT rc = {};
			GetClientRect(
				window,
				&rc
				);

			size = D2D1::SizeF(
				static_cast<FLOAT>((rc.right - rc.left) / dpiScaleX),
				static_cast<FLOAT>((rc.bottom - rc.top) / dpiScaleY)
			);


			std::tie(hr, wiadevmgr2) = lib::wr::ComCreateInstance<IWiaDevMgr2>(CLSID_WiaDevMgr2);
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

			PROPSPEC query[] = {
				{PRSPEC_PROPID, WIA_DIP_DEV_ID},
				{PRSPEC_PROPID, WIA_DIP_DEV_NAME},
				{PRSPEC_PROPID, WIA_DIP_DEV_DESC}
			};

			std::wstring text;
			while (unique_hresult::make(wiaenumdevinfo->Next( 1, wiapropertystorage.replace(), NULL )).suppress() == unique_hresult::cast(S_OK))
			{
				PROPVARIANT result[3] = {};
				auto resultRange = lib::rng::make_range(result);
				ON_UNWIND_AUTO([&]{FreePropVariantArray(resultRange.size(), resultRange.begin());});

				hr.reset(wiapropertystorage->ReadMultiple(resultRange.size(), query, resultRange.begin()));
				if (!hr)
				{
					return FALSE;
				}

				std::for_each(resultRange.begin(), resultRange.end(),
					[&] (decltype(resultRange[0]) item)
					{
						if (item.vt == VT_BSTR)
						{
							text += item.bstrVal;
							text += L"\n";
						}
					}
				);
			}
			label.text.set(text);

			Layout();

			return TRUE;
			UNREFERENCED_PARAMETER(context);
		}

		LRESULT OnCommand(const lib::wnd::Context<type>* context, HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
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

		LRESULT OnSize(const lib::wnd::Context<type>* context, HWND hwnd, UINT state, int cx, int cy)
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

		LRESULT OnPaint(const lib::wnd::Context<type>* context, HWND hwnd)
		{
			LRESULT lresult = 0;
			TRACE_SCOPE("lresult: %d", lresult);

			PAINTSTRUCT ps = {};
			HDC hdc = NULL;

			hdc = BeginPaint(hwnd, &ps);
			lib::wr::unique_gdi_end_paint ender(std::make_pair(hwnd, &ps));

			DrawD2DContent().suppress();

			return lresult;
			UNREFERENCED_PARAMETER(context);
		}

		LRESULT OnDestroy(const lib::wnd::Context<type>* context, HWND hwnd)
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

	lib::wnd::window_class_traits_builder<type> window_class_traits(tag&&);

	void window_class_register(PCWSTR windowClass, WNDCLASSEX* wcex, tag&&)
	{
		wcex->hIcon = LoadIcon(wcex->hInstance, MAKEINTRESOURCE(IDI_SCANNER));
		wcex->hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex->lpszMenuName = MAKEINTRESOURCE(IDC_SCANNER);
		wcex->lpszClassName = windowClass;
		wcex->hIconSm = LoadIcon(wcex->hInstance, MAKEINTRESOURCE(IDI_SMALL));
	}

	typedef
		lib::wnd::window_class<tag>
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
std::pair<unique_winerror, lib::wr::unique_close_window> 
CreateMainWindow(HINSTANCE hInstance, PCWSTR windowClass, PCWSTR title, int nCmdShow)
{
	unique_winerror winerror;
	TRACE_SCOPE("%!WINERROR!", winerror.get());

	MainWindow::registrar::Register(windowClass);

	lib::wr::unique_close_window window;

	std::tie(winerror, window) = lib::wr::winerror_and_close_window(
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
