#ifndef _ARG_PARSER_ARGPARSER_H_
#define _ARG_PARSER_ARGPARSER_H_

#include <vector>
#include <string>
#include <optional>
#include <functional>


namespace stypox {
	template <class IntType, class FloatType, class TextType,
		class = typename std::enable_if_t<
			std::is_integral_v<IntType> &&
			std::is_floating_point_v<FloatType> &&
			std::is_convertible_v<const char*, TextType> ||
			std::is_constructible_v<TextType, const char *>>>
	class BasicArgParser {
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
					 bool required = false,
					 T defaultValue = {});
			
			Argument(const std::string& name,
					 const std::string& description,
					 const std::vector<std::string>& parameters,
					 std::function<bool(std::enable_if_t<!std::is_same_v<T, bool>,T>)> validityChecker,
					 bool required = false,
					 T defaultValue = {});
		};

	public:
		using BoolArg  = Argument<bool>;
		using IntArg   = Argument<IntType>;
		using FloatArg = Argument<FloatType>;
		using TextArg  = Argument<TextType>;

	private:
		const std::vector<BoolArg>  boolArgs;
		const std::vector<IntArg>   intArgs;
		const std::vector<FloatArg> floatArgs;
		const std::vector<TextArg>  textArgs;

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
}

#endif