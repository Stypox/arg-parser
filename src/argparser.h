#ifndef _ARG_PARSER_ARGPARSER_H_
#define _ARG_PARSER_ARGPARSER_H_

#include <vector>
#include <string>
#include <optional>
#include <functional>


namespace stypox {
	template <class T>
	class Argument {
	public:
		using value_type = T;
	private:
		std::string m_name;
		std::string m_description;

		std::vector<std::string> m_parameters;
		std::optional<std::function<bool(T)>> m_validityChecker;
		bool m_required;

		T m_value;
		bool m_alreadySeen = false;

	public:
		Argument(const std::string& name,
				 const std::string& description,
				 const std::vector<std::string>& parameters,
				 const std::function<bool(T)>& validityChecker = [](T){ return true; },
				 T defaultValue = {},
				 bool required = false);
		
		bool operator== (const std::string_view& arg);
		void operator= (const std::string_view& arg);

		std::string name();
		T value();
	};

	template <class IntType, class FloatType, class TextType,
		class = typename std::enable_if_t<
			std::is_integral_v<IntType> &&
			std::is_floating_point_v<FloatType> &&
			std::is_assignable_v<TextType&, std::string>
		>
	>
	class BasicArgParser {
	public:
		using BoolArg  = Argument<bool>;
		using IntArg   = Argument<IntType>;
		using FloatArg = Argument<FloatType>;
		using TextArg  = Argument<TextType>;

	private:
		std::vector<BoolArg>  m_boolArgs;
		std::vector<IntArg>   m_intArgs;
		std::vector<FloatArg> m_floatArgs;
		std::vector<TextArg>  m_textArgs;

		std::string m_programName;

		template<class T>
		static bool findAssign(std::vector<Argument<T>>& typeArgs, const std::string_view& arg);

	public:
		BasicArgParser(const std::vector<BoolArg>&  boolArgs,
					   const std::vector<IntArg>&   intArgs,
					   const std::vector<FloatArg>& floatArgs,
					   const std::vector<TextArg>&  textArgs);

		void parse(int argc, char const* argv[]);

		bool      getBool(const std::string& name);
		IntType   getInt(const std::string& name);
		FloatType getFloat(const std::string& name);
		TextType  getText(const std::string& name);
	};

	using ArgParser = BasicArgParser<int, float, std::string>;
		
	
	template <class T>
	Argument<T>::
	Argument(
		const std::string& name,
		const std::string& description,
		const std::vector<std::string>& parameters,
		const std::function<bool(T)>& validityChecker,
		T defaultValue,
		bool required) :
		m_name{name}, m_description{description},
		m_parameters{parameters}, m_validityChecker{validityChecker},
		m_required{required}, m_value{defaultValue} {}
	
	template <class T>
	bool Argument<T>::
	operator== (const std::string_view& arg) {
		if constexpr(std::is_same_v<T, bool>) {
			return std::find(m_parameters.begin(), m_parameters.end(), arg) != m_parameters.end();
		}
		else {
			for (auto&& parameter : m_parameters) {
				if (arg.size() > parameter.size() && arg.substr(0, parameter.size()) == parameter)
					return true;
			}
			return false;
		}
	}
	template <class T>
	void Argument<T>::
	operator= (const std::string_view& arg) {
		if (m_alreadySeen)
			throw std::runtime_error("Argument \"" + m_name + "\" repeated multiple times: " + std::string{arg});
		m_alreadySeen = true;

		if constexpr(std::is_same_v<bool, T>) {
			m_value = true;
		}
		else {
			std::string argValue;
			for (auto&& parameter : m_parameters) {
				if (arg.size() > parameter.size() && arg.substr(0, parameter.size()) == parameter) {
					argValue = arg.substr(parameter.size());
					break;
				}
			}
			if (argValue.empty())
				throw std::runtime_error("Argument \"" + m_name + "\" requires a value: " + std::string{arg});
			

			if constexpr(std::is_integral_v<T>)
				m_value = std::stoll(argValue);
			else if constexpr(std::is_floating_point_v<T>)
				m_value = std::stold(argValue);
			else
				m_value = argValue;
		}
	}

	template <class T>
	std::string Argument<T>::
	name() {
		return m_name;
	}
	template <class T>
	T Argument<T>::
	value() {
		return m_value;
	}

	template <class IntType, class FloatType, class TextType, class Enable>
	template <class T>
	bool BasicArgParser<IntType, FloatType, TextType, Enable>::
	findAssign(std::vector<Argument<T>>& typeArgs, const std::string_view& arg) {
		if (auto found = std::find(typeArgs.begin(), typeArgs.end(), arg); found == typeArgs.end()) {
			return false;
		}
		else {
			(*found) = arg;
			return true;
		}
	}

	template <class IntType, class FloatType, class TextType, class Enable>
	BasicArgParser<IntType, FloatType, TextType, Enable>::
	BasicArgParser(
		const std::vector<BoolArg>&  boolArgs,
		const std::vector<IntArg>&   intArgs,
		const std::vector<FloatArg>& floatArgs,
		const std::vector<TextArg>&  textArgs) :
		m_boolArgs{boolArgs}, m_intArgs{intArgs},
		m_floatArgs{floatArgs}, m_textArgs{textArgs} {}

	template <class IntType, class FloatType, class TextType, class Enable>
	void BasicArgParser<IntType, FloatType, TextType, Enable>::
	parse(int argc, char const* argv[]) {
		m_programName = argv[0];
		for (int argIt = 1; argIt < argc; ++argIt) {
			std::string_view arg{argv[argIt]};
			if (!(findAssign(m_boolArgs, arg) ||
				  findAssign(m_intArgs, arg) ||
				  findAssign(m_floatArgs, arg) ||
				  findAssign(m_textArgs, arg)))
				throw std::runtime_error("Unknown argument: " + std::string{arg});
		}
	}

	template <class IntType, class FloatType, class TextType, class Enable>
	bool BasicArgParser<IntType, FloatType, TextType, Enable>::
	getBool(const std::string& name) {
		for(auto&& arg : m_boolArgs) {
			if (arg.name() == name)
				return arg.value();
		}
		throw std::out_of_range("stypox::BasicArgParser::getBool(): argument " + name + " not found");
	}	
	template <class IntType, class FloatType, class TextType, class Enable>
	IntType BasicArgParser<IntType, FloatType, TextType, Enable>::
	getInt(const std::string& name) {
		for(auto&& arg : m_intArgs) {
			if (arg.name() == name)
				return arg.value();
		}
		throw std::out_of_range("stypox::BasicArgParser::getInt(): argument " + name + " not found");
	}	
	template <class IntType, class FloatType, class TextType, class Enable>
	FloatType BasicArgParser<IntType, FloatType, TextType, Enable>::
	getFloat(const std::string& name) {
		for(auto&& arg : m_floatArgs) {
			if (arg.name() == name)
				return arg.value();
		}
		throw std::out_of_range("stypox::BasicArgParser::getFloat(): argument " + name + " not found");
	}
	template <class IntType, class FloatType, class TextType, class Enable>
	TextType BasicArgParser<IntType, FloatType, TextType, Enable>::
	getText(const std::string& name) {
		for(auto&& arg : m_textArgs) {
			if (arg.name() == name)
				return arg.value();
		}
		throw std::out_of_range("stypox::BasicArgParser::getInt(): argument " + name + " not found");
	}
}

#endif