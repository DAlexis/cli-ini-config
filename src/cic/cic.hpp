#ifndef LIBHEADER_INCLUDED
#define LIBHEADER_INCLUDED

#include "utils.hpp"
#include <boost/program_options.hpp>
#include <stdexcept>
#include <string>
#include <list>
#include <memory>
#include <iostream>

#ifndef DEBUG
	#define DEBUG
#endif

#ifdef DEBUG
    #define CIC_ASSERT(condition, message) if (not (condition)) throw std::runtime_error(std::string((message)));
#else
    #define CIC_ASSERT(condition, message)
#endif

namespace cic {

class IAnyTypeParameter
{
public:
	virtual ~IAnyTypeParameter() {}
	virtual std::string toString() const = 0;
	virtual const std::string& name() const = 0;

	virtual void addToPO(boost::program_options::options_description& od) const = 0;
	virtual bool getFromPO(const boost::program_options::variables_map& clOpts) = 0;

	virtual void writeIniItem(std::ostream& stream) = 0;

	virtual bool initialized() const = 0;
	virtual bool setByUser() const = 0;

	virtual IAnyTypeParameter* copy() const = 0;
};

template <typename T>
struct Parameter : public IAnyTypeParameter
{
	Parameter(const Parameter& p)
	{
		*this = p;
	}

	Parameter(const char* name, const char* description, T initValue) :
		m_value(initValue),
		m_name(name),
		m_description(description),
		m_isInitialized(true)
	{ }

	Parameter(const char* name, const char* description) :
		m_value(0.0),
		m_name(name),
		m_description(description),
		m_isInitialized(false)
	{ }

	T operator=(T newValue)
	{
		m_isInitialized = true;
		return m_value = newValue;
	}

	T& get()
	{
		CIC_ASSERT(m_isInitialized, std::string("Parameter ") + m_name + " usage without initialization!");
		return m_value;
	}

	const std::string& name() const override { return m_name; }

	std::string toString() const override
	{
		return StringTool<T>::to_string(m_value);
	}

	Parameter& operator=(const Parameter& right)
	{
		m_isInitialized = right.m_isInitialized;
		m_value = right.m_value;
		return *this;
	}

	void addToPO(boost::program_options::options_description& od) const override
	{
		if (m_isInitialized)
			od.add_options()
				(m_name.c_str(), boost::program_options::value<T>()->default_value(m_value), "Description");
		else
			od.add_options()
				(m_name.c_str(), boost::program_options::value<T>(), "Description");
	}

	bool getFromPO(const boost::program_options::variables_map& clOpts) override
	{
		if (clOpts.count(m_name.c_str()) != 0)
		{
			m_value = clOpts[m_name.c_str()].as<T>();
			m_setByUser = true;
		}
		return setByUser();
	}

	void writeIniItem(std::ostream& stream) override
	{
		stream << "# " << m_description << std::endl;
		stream << m_name << " = " << m_value << std::endl;
	}

	bool initialized() const override
	{
		return m_isInitialized;
	}

	bool setByUser() const override
	{
		return m_setByUser;
	}

	virtual IAnyTypeParameter* copy() const override
	{
		Parameter *p = new Parameter(*this);
		return p;
	}

private:
	T m_value;
	const std::string m_name;
	const std::string m_description;
	bool m_isInitialized;
	bool m_setByUser = false;
};

class ParametersGroup
{
public:
	void add(const IAnyTypeParameter& parameter);
	void readPO(const boost::program_options::variables_map& clOpts);
private:
	std::list<std::unique_ptr<IAnyTypeParameter>> m_parameters;
};


} // namespace cic

void func();
double sqr(double x);

#endif // LIBHEADER_INCLUDED
