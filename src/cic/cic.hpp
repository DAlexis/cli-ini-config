#ifndef LIBHEADER_INCLUDED
#define LIBHEADER_INCLUDED

#include "utils.hpp"
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <stdexcept>
#include <string>
#include <list>
#include <memory>
#include <iostream>

#include <initializer_list>

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
	virtual bool getFromPT(const boost::property_tree::ptree& pt) = 0;

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
			m_isInitialized = true;
		}
		return initialized();
	}

	bool getFromPT(const boost::property_tree::ptree& pt)
	{
		if (pt.count(m_name.c_str()) != 0)
		{
			m_value = pt.get<T>(m_name.c_str());
			m_setByUser = true;
			m_isInitialized = true;
		}

		return initialized();
	}

	void writeIniItem(std::ostream& stream) override
	{
		stream << "# " << m_description << std::endl;
		stream << m_name << " = " << m_value << std::endl;
	}

	bool initialized() const override {	return m_isInitialized; }

	bool setByUser() const override { return m_setByUser; }

	virtual IAnyTypeParameter* copy() const override
	{
		Parameter *p = new Parameter(*this);
		return p;
	}

private:
	T m_value;
	std::string m_name;
	std::string m_description;
	bool m_isInitialized;
	bool m_setByUser = false;
};

class ParametersGroup
{
public:
	ParametersGroup(const char* groupName, const char* description = "");

	template <typename... Args>
	ParametersGroup(const char* groupName, const char* description, Args... args) :
		m_optionsDescr(description),
		m_groupName(groupName),
		m_description(description)
	{
		add(args...);
	}

	template <typename... Args>
	ParametersGroup(const char* groupName, Args... args) :
		m_groupName(groupName)
	{
		add(args...);
	}

	ParametersGroup(ParametersGroup&& pg);
	const std::string& name();

	template <typename... Args>
	void add(const IAnyTypeParameter& parameter, Args... args)
	{
		add(parameter);
		add(args...);
	}
	void add(const IAnyTypeParameter& parameter);

	void readPOVarsMap(const boost::program_options::variables_map& clOpts);
	const boost::program_options::options_description& getOptionsDesctiption();

	/**
	 * Read variables from boost::property_tree
	 * returns false if at least one parameter in this group is not initialized yet
	 */
	bool readPT(const boost::property_tree::ptree& pt);
	void writeIniItem(std::ostream& stream);

	IAnyTypeParameter& getInterface(const std::string& name);

	template <typename T>
	T& get(const std::string& name)
	{
		return dynamic_cast<Parameter<T>&>(getInterface(name)).get();
	}

private:
	boost::program_options::options_description m_optionsDescr;

	bool areAllInitialized();
	std::string m_groupName;
	std::string m_description;
	std::map<std::string, std::unique_ptr<IAnyTypeParameter>> m_parameters;
};

class Parameters
{
public:
	Parameters(const char* title = "Allowed options");

	template <typename... Args>
	Parameters(const char* title, Args&&... args) :
		m_allOptions(title)
	{
		addGroup(std::forward<Args>(args)...);
	}

	template <typename... Args>
	void addGroup(ParametersGroup&& parameter, Args&&... args)
	{
		addGroup(std::move(parameter));
		addGroup(std::forward<Args>(args)...);
	}

	template <typename... Args>
	void addGroup(ParametersGroup& parameter, Args&&... args)
	{
		addGroup(parameter);
		addGroup(std::forward<Args>(args)...);
	}

	void addGroup(ParametersGroup&& pg);
	void addGroup(ParametersGroup& pg);

	void parseCmdline(int argc, const char** argv);
	void parseIni(const char* filename);

	void cmdlineHelp(std::ostream& stream);

	ParametersGroup& operator[](const std::string& groupName);

private:
	std::list<std::unique_ptr<ParametersGroup>> m_pgOwners;
	std::map<std::string, ParametersGroup*> m_groups;
	boost::program_options::options_description m_allOptions;
	boost::program_options::variables_map m_vm;
	boost::property_tree::ptree m_pt;
};


} // namespace cic

void func();
double sqr(double x);

#endif // LIBHEADER_INCLUDED
