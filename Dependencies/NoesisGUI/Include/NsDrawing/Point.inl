////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Math.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Point::Point(): Vector2(0.0f, 0.0f)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Point::Point(float x, float y): Vector2(x, y)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Point::Point(const Pointi& point): Vector2((float)point.x, (float)point.y)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Point::Point(const Vector2& v): Vector2(v)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Point::Point(const Size& size): Vector2(size.width, size.height)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Pointi::Pointi(): x(0), y(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Pointi::Pointi(int xx, int yy): x(xx), y(yy)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Pointi::Pointi(const Point& p): x(Round(p.x)), y(Round(p.y))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Pointi::Pointi(const Sizei& size) : x(size.width), y(size.height)
{
}

}
