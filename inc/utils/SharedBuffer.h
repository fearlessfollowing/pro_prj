#ifndef ANDROID_SHARED_BUFFER_H
#define ANDROID_SHARED_BUFFER_H

#include <stdint.h>
#include <sys/types.h>
#include <atomic>

namespace android {

class SharedBuffer {
public:

    /* flags to use with release() */
    enum {
        eKeepStorage = 0x00000001
    };

    /*! allocate a buffer of size 'size' and acquire() it.
     *  call release() to free it.
     */
    static          SharedBuffer*           alloc(size_t size);
    
    /*! free the memory associated with the SharedBuffer.
     * Fails if there are any users associated with this SharedBuffer.
     * In other words, the buffer must have been release by all its
     * users.
     */
    static          ssize_t                 dealloc(const SharedBuffer* released);

    //! access the data for read
    inline          const void*             data() const;
    
    //! access the data for read/write
    inline          void*                   data();

    //! get size of the buffer
    inline          size_t                  size() const;
 
    //! get back a SharedBuffer object from its data
    static  inline  SharedBuffer*           bufferFromData(void* data);
    
    //! get back a SharedBuffer object from its data
    static  inline  const SharedBuffer*     bufferFromData(const void* data);

    //! get the size of a SharedBuffer object from its data
    static  inline  size_t                  sizeFromData(const void* data);
    
    //! edit the buffer (get a writtable, or non-const, version of it)
                    SharedBuffer*           edit();

    //! edit the buffer, resizing if needed
                    SharedBuffer*           editResize(size_t size);

    //! like edit() but fails if a copy is required
                    SharedBuffer*           attemptEdit() const;
    
    //! resize and edit the buffer, loose it's content.
                    SharedBuffer*           reset(size_t size);

    //! acquire/release a reference on this buffer
                    void                    acquire();
                    
    /*! release a reference on this buffer, with the option of not
     * freeing the memory associated with it if it was the last reference
     * returns the previous reference count
     */     
                    int32_t                 release(uint32_t flags = 0);
    
    //! returns wether or not we're the only owner
    inline          bool                    onlyOwner() const;
    

private:
        inline SharedBuffer() { }
        inline ~SharedBuffer() { }
        SharedBuffer(const SharedBuffer&);
        SharedBuffer& operator = (const SharedBuffer&);
 
        // 16 bytes. must be sized to preserve correct alignment.
        // mutable int32_t        mRefs;
                std::atomic<int> mRefs;
                size_t          mSize;
                uint32_t        mReserved[2];
};

// ---------------------------------------------------------------------------

const void* SharedBuffer::data() const 
{
    return this + 1;
}

void* SharedBuffer::data() 
{
    return this + 1;
}

size_t SharedBuffer::size() const 
{
    return mSize;
}

SharedBuffer* SharedBuffer::bufferFromData(void* data) 
{
    return data ? static_cast<SharedBuffer *>(data)-1 : 0;
}
    
const SharedBuffer* SharedBuffer::bufferFromData(const void* data) 
{
    return data ? static_cast<const SharedBuffer *>(data)-1 : 0;
}

size_t SharedBuffer::sizeFromData(const void* data) 
{
    return data ? bufferFromData(data)->mSize : 0;
}

bool SharedBuffer::onlyOwner() const 
{
    return (mRefs == 1);
}

}; // namespace android


#endif // ANDROID_VECTOR_H
