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

	public:
		Argument(const std::string& name,
				 const std::string& description,
				 const std::vector<std::string>& parameters,
				 const std::function<bool(T)>& validityChecker = [](T){ return true; },
				 T defaultValue = {},
				 bool required = false);
	};

	template <class IntType, class FloatType, class TextType,
		class = typename std::enable_if_t<
			std::is_integral_v<IntType> &&
			std::is_floating_point_v<FloatType> && (
				std::is_convertible_v<const char*, TextType> ||
				std::is_constructible_v<TextType, const char *>
			)
		>
	>
	class BasicArgParser {
	public:
		using BoolArg  = Argument<bool>;
		using IntArg   = Argument<IntType>;
		using FloatArg = Argument<FloatType>;
		using TextArg  = Argument<TextType>;

	private:
		const std::vector<BoolArg>  m_boolArgs;
		const std::vector<IntArg>   m_intArgs;
		const std::vector<FloatArg> m_floatArgs;
		const std::vector<TextArg>  m_textArgs;

	public:
		BasicArgParser(const std::vector<BoolArg>&  boolArgs,
					   const std::vector<IntArg>&   intArgs,
					   const std::vector<FloatArg>& floatArgs,
					   const std::vector<TextArg>&  textArgs);

		void parse(int argc, char const* argv[]);

		bool      getBool();
		IntType   getInt();
		FloatType getFloat();
		TextType  getText();
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


	template <class IntType, class FloatType, class TextType, class Enable>
	BasicArgParser<IntType, FloatType, TextType, Enable>::
	BasicArgParser(
		const std::vector<BoolArg>&  boolArgs,
		const std::vector<IntArg>&   intArgs,
		const std::vector<FloatArg>& floatArgs,
		const std::vector<TextArg>&  textArgs) :
		m_boolArgs{boolArgs}, m_intArgs{intArgs},
		m_floatArgs{floatArgs}, m_textArgs{textArgs} {}
}

#endif