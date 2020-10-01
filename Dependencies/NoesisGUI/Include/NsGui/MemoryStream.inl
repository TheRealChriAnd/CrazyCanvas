////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Math.h>
#include <string.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
inline MemoryStream::MemoryStream(const void *buffer, uint32_t size) : mBuffer(buffer), mSize(size),
    mOffset(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void MemoryStream::SetPosition(uint32_t pos)
{
    mOffset = pos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint32_t MemoryStream::GetPosition() const
{
    return mOffset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint32_t MemoryStream::GetLength() const
{
    return mSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint32_t MemoryStream::Read(void* buffer, uint32_t size)
{
    NS_ASSERT(size == 0 || buffer != 0);
    uint32_t read = Min(size, mSize - mOffset);
    memcpy(buffer, (const uint8_t*)mBuffer + mOffset, read);
    mOffset += read;
    return read;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void MemoryStream::Close()
{
}

}
