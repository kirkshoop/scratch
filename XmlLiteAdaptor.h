#include <xmllite.h>

typedef
	lib::wr::unique_com_interface<IStream>::type
unique_com_stream;

typedef
	lib::wr::unique_com_interface<IXmlReader>::type
unique_com_xmlreader;

namespace com
{
	namespace detail
	{
		namespace ifset=lib::ifset;

		struct default_sequentialstream
		{
			void interface_constructed(ISequentialStream*, ifset::interface_tag<ISequentialStream>&&) 
			{}

			// ISequentialStream Interface

			inline
			std::pair<unique_hresult, lib::rng::range<PBYTE>>
			STDMETHODCALLTYPE Read(lib::rng::range<PBYTE>)
			{
				return std::make_pair(
					hresult_cast(E_NOTIMPL), 
					lib::rng::range<PBYTE>()
				);
			}

			inline
			std::pair<unique_hresult, lib::rng::range<const BYTE*>>
			STDMETHODCALLTYPE Write(lib::rng::range<const BYTE*> bytes)
			{
				return std::make_pair(
					hresult_cast(E_NOTIMPL), 
					bytes
				);
			}
		};

		template<typename ComObjectTag, typename Base>
		struct com_sequentialstream
			: public Base
		{
		private:
			typedef
				com_sequentialstream
			this_type;

			typedef
				ISequentialStream
			interface_type;

			typedef
				ComObjectTag
			object_tag;

			typedef
				ifset::interface_tag<interface_type>
			interface_tag;

			typedef
				decltype(interface_storage(lib::cmn::instance_of<this_type*>::value, interface_tag(), object_tag()))
			storage_type;

			storage_type storage()
			{
				return interface_storage(this, interface_tag(), object_tag());
			}

			template<typename Function>
			static HRESULT hresult_contract(Function&& function)
			{
				return com_function_contract_hresult(
					std::forward<Function>(function), 
					interface_tag(), 
					object_tag());
			}

		public:
			~com_sequentialstream()
			{}

			com_sequentialstream()
			{}

			template<TPLT_TEMPLATE_ARGUMENTS_DECL(1, Param)>
			explicit com_sequentialstream(TPLT_FUNCTION_ARGUMENTS_DECL(1, Param, , &&))
				: Base(TPLT_FUNCTION_ARGUMENTS_CAST(1, Param, std::forward))
			{}

			template<TPLT_TEMPLATE_ARGUMENTS_DECL(2, Param)>
			com_sequentialstream(TPLT_FUNCTION_ARGUMENTS_DECL(2, Param, , &&))
				: Base(TPLT_FUNCTION_ARGUMENTS_CAST(2, Param, std::forward))
			{}

			template<TPLT_TEMPLATE_ARGUMENTS_DECL(3, Param)>
			com_sequentialstream(TPLT_FUNCTION_ARGUMENTS_DECL(3, Param, , &&))
				: Base(TPLT_FUNCTION_ARGUMENTS_CAST(3, Param, std::forward))
			{}

			template<TPLT_TEMPLATE_ARGUMENTS_DECL(4, Param)>
			com_sequentialstream(TPLT_FUNCTION_ARGUMENTS_DECL(4, Param, , &&))
				: Base(TPLT_FUNCTION_ARGUMENTS_CAST(4, Param, std::forward))
			{}

			HRESULT STDMETHODCALLTYPE Read(void* data, ULONG size, ULONG* outRead)
			{
				*outRead = 0;
				return hresult_contract(
					[&]() -> unique_hresult
					{
						PBYTE bytes = reinterpret_cast<PBYTE>(data);
						unique_hresult result;
						lib::rng::range<PBYTE> outReadRange;
						std::tie(result, outReadRange) = 
							storage()->Read(lib::rng::make_range(bytes, bytes + size));
						*outRead = static_cast<ULONG>(outReadRange.size());
						return result;
					}
				);
			}

			HRESULT STDMETHODCALLTYPE Write(const void* data, ULONG size, ULONG* outWritten)
			{
				*outWritten = 0;
				return hresult_contract(
					[&]() -> unique_hresult
					{
						const BYTE* bytes = reinterpret_cast<const BYTE*>(data);
						unique_hresult result;
						lib::rng::range<const BYTE*> bytesRemaining;
						std::tie(result, bytesRemaining) = 
							storage()->Write(lib::rng::make_range(bytes, bytes + size));
						*outWritten = static_cast<ULONG>(size - bytesRemaining.size());
						return result;
					}
				);
			}
		};

		struct default_stream
		{
			void interface_constructed(IStream*, ifset::interface_tag<IStream>&&) 
			{}

			// IStream Interface

			inline
			unique_hresult STDMETHODCALLTYPE SetSize(ULARGE_INTEGER)
			{ 
				return hresult_cast(E_NOTIMPL);   
			}
    
			inline
			unique_hresult STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*,
				ULARGE_INTEGER*) 
			{ 
				return hresult_cast(E_NOTIMPL);   
			}

			inline
			unique_hresult STDMETHODCALLTYPE Commit(DWORD)                                      
			{ 
				return hresult_cast(E_NOTIMPL);   
			}

			inline
			unique_hresult STDMETHODCALLTYPE Revert(void)                                       
			{ 
				return hresult_cast(E_NOTIMPL);   
			}
    
			inline
			unique_hresult STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)              
			{ 
				return hresult_cast(E_NOTIMPL);   
			}
    
			inline
			unique_hresult STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)            
			{ 
				return hresult_cast(E_NOTIMPL);   
			}
    
			inline
			unique_hresult STDMETHODCALLTYPE Clone(IStream **)                                  
			{ 
				return hresult_cast(E_NOTIMPL);   
			}

			inline
			unique_hresult STDMETHODCALLTYPE Seek(LARGE_INTEGER , DWORD ,
				ULARGE_INTEGER* )
			{ 
				return hresult_cast(E_NOTIMPL);   
			}

			inline
			unique_hresult STDMETHODCALLTYPE Stat(STATSTG* , DWORD ) 
			{
				return hresult_cast(E_NOTIMPL);   
			}
		};

		template<typename ComObjectTag, typename Base>
		struct com_stream
			: public Base
		{
		private:
			typedef
				com_stream
			this_type;

			typedef
				IStream
			interface_type;

			typedef
				ComObjectTag
			object_tag;

			typedef
				ifset::interface_tag<interface_type>
			interface_tag;

			typedef
				decltype(interface_storage(lib::cmn::instance_of<this_type*>::value, interface_tag(), object_tag()))
			storage_type;

			storage_type storage()
			{
				return interface_storage(this, interface_tag(), object_tag());
			}

			template<typename Function>
			static HRESULT hresult_contract(Function&& function)
			{
				return com_function_contract_hresult(
					std::forward<Function>(function), 
					interface_tag(), 
					object_tag());
			}

		public:
			~com_stream()
			{}

			com_stream()
			{}

			template<TPLT_TEMPLATE_ARGUMENTS_DECL(1, Param)>
			explicit com_stream(TPLT_FUNCTION_ARGUMENTS_DECL(1, Param, , &&))
				: Base(TPLT_FUNCTION_ARGUMENTS_CAST(1, Param, std::forward))
			{}

			template<TPLT_TEMPLATE_ARGUMENTS_DECL(2, Param)>
			com_stream(TPLT_FUNCTION_ARGUMENTS_DECL(2, Param, , &&))
				: Base(TPLT_FUNCTION_ARGUMENTS_CAST(2, Param, std::forward))
			{}

			template<TPLT_TEMPLATE_ARGUMENTS_DECL(3, Param)>
			com_stream(TPLT_FUNCTION_ARGUMENTS_DECL(3, Param, , &&))
				: Base(TPLT_FUNCTION_ARGUMENTS_CAST(3, Param, std::forward))
			{}

			template<TPLT_TEMPLATE_ARGUMENTS_DECL(4, Param)>
			com_stream(TPLT_FUNCTION_ARGUMENTS_DECL(4, Param, , &&))
				: Base(TPLT_FUNCTION_ARGUMENTS_CAST(4, Param, std::forward))
			{}

			HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size)
			{ 
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->SetSize(size);
					}
				);
			}
    
			HRESULT STDMETHODCALLTYPE CopyTo(IStream* from, ULARGE_INTEGER size, ULARGE_INTEGER* read,
				ULARGE_INTEGER* written) 
			{ 
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->CopyTo(from, size, read, written);
					}
				);
			}
    
			HRESULT STDMETHODCALLTYPE Commit(DWORD flags)                                      
			{ 
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->Commit(flags);
					}
				);
			}
    
			inline
			virtual HRESULT STDMETHODCALLTYPE Revert(void)                                       
			{ 
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->Revert();
					}
				);
			}
    
			inline
			virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD lockType)              
			{ 
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->LockRegion(offset, size, lockType);
					}
				);
			}
    
			inline
			virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD lockType)            
			{ 
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->UnlockRegion(offset, size, lockType);
					}
				);
			}
    
			inline
			virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** outStream)                                  
			{ 
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->Clone(outStream);
					}
				);
			}

			inline
			virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER distance, DWORD origin,
				ULARGE_INTEGER* outLocation)
			{ 
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->Seek(distance, origin, outLocation);
					}
				);
			}

			inline
			virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* outStatus, DWORD flags) 
			{
				return hresult_contract(
					[&]() -> unique_hresult
					{
						return storage()->Stat(outStatus, flags);
					}
				);
			}
		};
	}
}

template<typename ComObjectTag>
COM_NAMESPACE::ifset::interface_traits_builder<com::detail::com_stream , com::detail::default_stream>
interface_tag_traits(COM_NAMESPACE::ifset::interface_tag<IStream>&&, ComObjectTag&&);

template<typename ComObjectTag>
COM_NAMESPACE::ifset::interface_traits_builder<com::detail::com_sequentialstream , com::detail::default_sequentialstream>
interface_tag_traits(COM_NAMESPACE::ifset::interface_tag<ISequentialStream>&&, ComObjectTag&&);




namespace com
{
	namespace file
	{
		struct tag {};

		struct type 
			: public lib::ifset::interface_traits<tag, IUnknown>::type::default_storage
			, public lib::ifset::interface_traits<tag, ISequentialStream>::type::default_storage
			, public lib::ifset::interface_traits<tag, IStream>::type::default_storage
		{
			using lib::ifset::interface_traits<tag, IUnknown>::type::default_storage::interface_constructed;
			using lib::ifset::interface_traits<tag, ISequentialStream>::type::default_storage::interface_constructed;
			using lib::ifset::interface_traits<tag, IStream>::type::default_storage::interface_constructed;

			explicit type(lib::wr::unique_file fileArg)
				: file(std::move(fileArg))
			{
   			}

			explicit type(
				LPCWSTR fileName, 
				DWORD mode = OPEN_EXISTING, 
				DWORD access = GENERIC_READ, 
				DWORD share = FILE_SHARE_READ, 
				DWORD attributes = FILE_ATTRIBUTE_NORMAL
			)
			{
				unique_winerror winerror;
				std::tie(winerror, file) = lib::wr::winerror_and_file(
					CreateFileW(
						fileName, 
						access, 
						share,
						NULL, 
						mode, 
						attributes, 
						NULL
					)
				);

				// only way to fail a constructor
				winerror.throw_if();
   			}

			// ISequentialStream Interface

			inline
			std::pair<unique_hresult, lib::rng::range<PBYTE>>
			STDMETHODCALLTYPE Read(lib::rng::range<PBYTE> bytes)
			{
				ULONG bytesRead = 0;
				unique_winerror winerror;
				winerror = make_winerror_if(
					!ReadFile(file.get(), bytes.begin(), bytes.size(), &bytesRead, NULL)
				);
				return std::make_pair(
					hresult_cast(HRESULT_FROM_WIN32(winerror.suppress().get())), 
					bytes.advance_end(bytesRead - bytes.size())
				);
			}

			inline
			std::pair<unique_hresult, lib::rng::range<const BYTE*>>
			STDMETHODCALLTYPE Write(lib::rng::range<const BYTE*> bytes)
			{
				ULONG bytesWritten = 0;
				unique_winerror winerror;
				winerror = make_winerror_if(
					!WriteFile(file.get(), bytes.begin(), bytes.size(), &bytesWritten, NULL)
				);
				return std::make_pair(
					hresult_cast(HRESULT_FROM_WIN32(winerror.suppress().get())), 
					bytes.advance_begin(bytes.size() - bytesWritten)
				);
			}

			// IStream Interface

			inline
			unique_hresult STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
				ULARGE_INTEGER* lpNewFilePointer)
			{ 
				DWORD dwMoveMethod;

				switch(dwOrigin)
				{
				case STREAM_SEEK_SET:
					dwMoveMethod = FILE_BEGIN;
					break;
				case STREAM_SEEK_CUR:
					dwMoveMethod = FILE_CURRENT;
					break;
				case STREAM_SEEK_END:
					dwMoveMethod = FILE_END;
					break;
				default:   
					return hresult_cast(STG_E_INVALIDFUNCTION);
					break;
				}

				if (SetFilePointerEx(file.get(), liDistanceToMove, (PLARGE_INTEGER) lpNewFilePointer,
									 dwMoveMethod) == 0)
					return hresult_cast(HRESULT_FROM_WIN32(GetLastError()));
				return hresult_cast(S_OK);
			}

			inline
			unique_hresult STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag) 
			{
				if (GetFileSizeEx(file.get(), (PLARGE_INTEGER) &pStatstg->cbSize) == 0)
					return hresult_cast(HRESULT_FROM_WIN32(GetLastError()));
				return hresult_cast(S_OK);
				UNREFERENCED_PARAMETER(grfStatFlag);
			}

		private:
			lib::wr::unique_file file;
			LONG refcount;
		};

		lib::ifset::traits_builder<
			typename lib::tv::factory<IUnknown, ISequentialStream, IStream>::type, 
			type> 
		interface_set_traits(tag&&);

	}
}

template<typename Function, typename InterfaceTag>
HRESULT
com_function_contract_hresult(Function&& function, InterfaceTag&&, com::file::tag&&)
{
	HRESULT hr = S_OK;
	FAIL_FAST_ON_THROW(
		[&]
		{
			try
			{
				hr = std::forward<Function>(function)().suppress().get();
			}
			catch(const unique_hresult::exception& e)
			{
				hr = HRESULT_FROM_WIN32(e.get());
			}
			catch(const unique_winerror::exception& e)
			{
				hr = e.get();
			}
			catch(const std::bad_alloc&)
			{
				hr = E_OUTOFMEMORY;
			}
		}
	);
	return hr;
}

template<typename T, typename InterfaceTag>
com::file::type* 
interface_storage(
	T* that,
	InterfaceTag&&, 
	com::file::tag&&
)
{
	return static_cast<com::file::type*>(that);
}

namespace com
{
	namespace file
	{
		typedef
			lib::ifset::interface_set<tag>
		object;
	}
}


class FileStream : public IStream
{
	inline
    FileStream(HANDLE hFile) 
    {
        refcount = 1;
        file.reset(hFile);
    }

	inline
    ~FileStream()
    {
    }

public:
	inline
    HRESULT static OpenFile(LPCWSTR pName, IStream ** ppStream, bool fWrite = false)
    {
        HANDLE hFile = ::CreateFileW(pName, fWrite ? GENERIC_WRITE : GENERIC_READ, FILE_SHARE_READ,
            NULL, fWrite ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
            return HRESULT_FROM_WIN32(GetLastError());
        
        *ppStream = new (std::nothrow) FileStream(hFile);
        
        if(*ppStream == NULL)
            CloseHandle(hFile);
            
        return S_OK;
    }

	inline
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject)
    { 
        if (iid == __uuidof(IUnknown)
            || iid == __uuidof(IStream)
            || iid == __uuidof(ISequentialStream))
        {
            *ppvObject = static_cast<IStream*>(this);
            AddRef();
            return S_OK;
        } else
            return E_NOINTERFACE; 
    }

	inline
    virtual ULONG STDMETHODCALLTYPE AddRef(void) 
    { 
        return (ULONG)InterlockedIncrement(&refcount); 
    }

	inline
    virtual ULONG STDMETHODCALLTYPE Release(void) 
    {
        ULONG res = (ULONG) InterlockedDecrement(&refcount);
        if (res == 0) 
            delete this;
        return res;
    }

    // ISequentialStream Interface
public:
	inline
    virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead)
    {
        BOOL rc = ReadFile(file.get(), pv, cb, pcbRead, NULL);
        return (rc) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

	inline
    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten)
    {
        BOOL rc = WriteFile(file.get(), pv, cb, pcbWritten, NULL);
        return rc ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    // IStream Interface
public:
	inline
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER)
    { 
        return E_NOTIMPL;   
    }
    
	inline
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*,
        ULARGE_INTEGER*) 
    { 
        return E_NOTIMPL;   
    }
    
	inline
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD)                                      
    { 
        return E_NOTIMPL;   
    }
    
	inline
    virtual HRESULT STDMETHODCALLTYPE Revert(void)                                       
    { 
        return E_NOTIMPL;   
    }
    
	inline
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)              
    { 
        return E_NOTIMPL;   
    }
    
	inline
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)            
    { 
        return E_NOTIMPL;   
    }
    
	inline
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream **)                                  
    { 
        return E_NOTIMPL;   
    }

	inline
    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
        ULARGE_INTEGER* lpNewFilePointer)
    { 
        DWORD dwMoveMethod;

        switch(dwOrigin)
        {
        case STREAM_SEEK_SET:
            dwMoveMethod = FILE_BEGIN;
            break;
        case STREAM_SEEK_CUR:
            dwMoveMethod = FILE_CURRENT;
            break;
        case STREAM_SEEK_END:
            dwMoveMethod = FILE_END;
            break;
        default:   
            return STG_E_INVALIDFUNCTION;
            break;
        }

        if (SetFilePointerEx(file.get(), liDistanceToMove, (PLARGE_INTEGER) lpNewFilePointer,
                             dwMoveMethod) == 0)
            return HRESULT_FROM_WIN32(GetLastError());
        return S_OK;
    }

	inline
    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag) 
    {
        if (GetFileSizeEx(file.get(), (PLARGE_INTEGER) &pStatstg->cbSize) == 0)
            return HRESULT_FROM_WIN32(GetLastError());
        return S_OK;
		UNREFERENCED_PARAMETER(grfStatFlag);
    }

private:
    lib::wr::unique_file file;
    LONG refcount;
};
