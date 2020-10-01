////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
TypeClassMetaData::TypeClassMetaData(const TypeClass* tc, const TypeMetaData* m): typeClass(tc),
    metaData(m)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TypeClassProperty::TypeClassProperty(const TypeClass* tc, const TypeProperty* p): typeClass(tc),
    property(p)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TypeClassEvent::TypeClassEvent(const TypeClass* tc, const TypeProperty* e): typeClass(tc), event(e)
{
}

}
