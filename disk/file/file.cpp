#include "file.hpp"

disk::file::file        (std::string _name, file::access_mode _mode) : f_path(_name), disk::disk_object(disk::type::file)
{
#ifdef UNIX_MODE
    f_handle = open             (_name.c_str(), O_CREAT, _mode);
    if(f_handle < 0 && on_error) on_error(this, errno);

    struct stat _fstat;
    fstat       (f_handle, &_fstat);

    f_size = _fstat.st_size;

#else
    f_handle = CreateFile(_name.c_str(), _mode,
                          0, NULL, OPEN_ALWAYS, 0, NULL);
    
    if(f_handle < 0 && on_error) on_error(this, GetLastError());

    DWORD _hsize = 0;
    
    f_size       = GetFileSize(f_handle, &_hsize);
    f_size       = ((size_t)_hsize << 32) | f_size;
#endif
}

disk::file::~file()
{
	close();
}

size_t disk::file::read (uint8_t* r_ctx, size_t r_size)
{
#ifdef UNIX_MODE

    if(sync_mode == stream::stream_mode::sync) read_lock.acquire(); 
    size_t _sz    = ::read(f_handle, (void*)r_ctx, r_size);

    if(sync_mode == stream::stream_mode::sync) read_lock.release();
    if    (_sz   <= 0)
    {
        if(on_error) { on_error(this, errno); return _sz; }
        else         { return _sz; }
    }

    if    (on_read)  { on_read (this, r_ctx, _sz); return 0; }
    else                                           return _sz;

#else

    if(sync_mode == stream::stream_mode::sync) read_lock.acquire(); 
    size_t _sz;
    bool   r_success = ReadFile(f_handle, (void*)r_ctx, r_size, (LPDWORD)&_sz, NULL);

    if(sync_mode == stream::stream_mode::sync) read_lock.release();
    if    (!r_success)
    {
        if(on_error) { on_error(this, GetLastError()); return _sz; }
        else         { return _sz; }
    }

    if    (on_read)  { on_read (this, r_ctx, _sz); return 0; }
    else                                           return _sz;

#endif
}

size_t disk::file::write(uint8_t* w_ctx, size_t w_size)
{
#ifdef UNIX_MODE

    if(sync_mode == stream::stream_mode::sync) write_lock.acquire(); 
    size_t _sz    = ::write(f_handle, (void*)w_ctx, w_size);

    if(sync_mode == stream::stream_mode::sync) write_lock.release();
    if    (_sz   <= 0)
    {
        if(on_error) { on_error(this, errno); return _sz; }
        else         { return _sz; }
    }

        if    (on_write) { on_write(this, _sz); return 0; }
        else                                    return _sz;

#else

    if(sync_mode == stream::stream_mode::sync) write_lock.acquire(); 
    size_t _sz;
    bool   r_success = WriteFile(f_handle, (void*)w_ctx, w_size, (LPDWORD)&_sz, NULL);

    if(sync_mode == stream::stream_mode::sync) write_lock.release();
    if    (!r_success)
    {
        if(on_error) { on_error(this, GetLastError()); return _sz; }
        else         { return _sz; }
    }

    if    (on_write) { on_write(this, _sz); return 0; }
    else                                    return _sz;

#endif
}

void   disk::file::offset(size_t m_ptr)
{
#ifdef WIN32_MODE
	LONG _hptr   = m_ptr >> 32;
	SetFilePointer(f_handle,
				   m_ptr & 0xFFFFFFFF, &_hptr,
				   FILE_BEGIN);
#else
	lseek		  (f_handle, m_ptr, SEEK_SET);
#endif
}

void* disk::file::map  ()
{
    f_state = file::state::mapped;

#ifdef UNIX_MODE
    f_mmap_pointer = mmap(0, f_size, 
                          PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS, f_handle, 0);

	return f_mmap_pointer;

#else
    f_mmap_handle  = CreateFileMapping(INVALID_HANDLE_VALUE,
									   NULL,
                                       PAGE_READWRITE,
                                       (f_size >> 32),
                                       (f_size & 0xFFFFFFFF), NULL);

    f_mmap_pointer = MapViewOfFile(f_mmap_handle,
                                   FILE_MAP_ALL_ACCESS,
                                   0, 0, f_size);    
#endif

	return f_mmap_pointer;
}

void  disk::file::unmap()
{
    f_state = file::state::general;

#ifdef WIN32_MODE
    UnmapViewOfFile(f_mmap_pointer);
    CloseHandle    (f_mmap_handle);
#else
    munmap         (f_mmap_pointer, f_size);
#endif
}