/*
 * utils.hpp
 *
 *  Created on: 26 мая 2017 г.
 *      Author: dalexies
 */

#ifndef CIC_UTILS_HPP_
#define CIC_UTILS_HPP_

#include <string>

template <typename T>
class ToStringConverter
{
public:
	static std::string to_string(const T& v)
	{
		return std::to_string(v);
	}
};

template <>
class ToStringConverter<std::string>
{
public:
	static std::string to_string(const std::string& v)
	{
		return v;
	}
};


template <typename T>
class StringTool : public ToStringConverter<T>
{
};

#endif /* CIC_UTILS_HPP_ */
