////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


namespace Noesis
{

template<class T> struct TypeTag;

////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint32_t MetaData::Count() const
{
    return mMetaDatas.Size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline TypeMetaData* MetaData::Get(uint32_t index) const
{
    return mMetaDatas[index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T> inline T* MetaData::Find() const
{
    return static_cast<T*>(Find(T::StaticGetClassType((TypeTag<T>*)nullptr)));
}

}
