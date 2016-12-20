#ifndef _COMMON_TW_CAST_H
#define _COMMON_TW_CAST_H

#include <string>
#include <sstream>
#include <cstdlib>

template <typename Target, typename Source>
inline Target tw_cast(const Source source)
{
	Target target = Target();

	std::stringstream ss;
	ss << source;
	ss >> target;
	return target;
}

template <typename T>
inline T text2value(const char* text)
{
	T value;
	std::stringstream ss;
	ss << text;
	ss >> value;
	return value;
}


template <>
inline int text2value(const char* text) 
{
	return std::atoi(text);
}

#endif // _COMMON_TW_CAST_H