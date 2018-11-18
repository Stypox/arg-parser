#ifndef _ARG_PARSER_ARGPARSER_H_
#define _ARG_PARSER_ARGPARSER_H_

#include <vector>
#include <string>
#include <optional>
#include <functional>


namespace stypox {
	template <class T>
	class Option {
	public:
		using value_type = T;
	private:
		const std::string m_name;
		const std::string m_description;

		const std::vector<std::string> m_arguments;
		const std::function<bool(T)> m_validityChecker;
		const bool m_required;

		T m_defaultValue;
		T m_value;
		bool m_alreadySeen = false;

	public:
		Option(const std::string& name,
			   const std::string& description,
			   const std::vector<std::string>& arguments,
			   const std::optional<T>& defaultValue = std::optional<T>{T{}},
			   const std::function<bool(T)>& validityChecker = [](T){ return true; });
		
		bool operator== (const std::string_view& arg) const;
		void operator= (const std::string_view& arg);
		void checkValidity() const;
		void reset();

		std::string name() const;
		T value() const;

		std::string help(size_t descriptionIndentation) const;
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
		using BoolArg  = Option<bool>;
		using IntArg   = Option<IntType>;
		using FloatArg = Option<FloatType>;
		using TextArg  = Option<TextType>;

	private:
		std::vector<BoolArg>  m_boolOptions;
		std::vector<IntArg>   m_intOptions;
		std::vector<FloatArg> m_floatOptions;
		std::vector<TextArg>  m_textOptions;

		const std::string m_programName;
		std::optional<std::string> m_executablePath;

		size_t m_descriptionIndentation;

		template <class T>
		static bool findAssign(std::vector<Option<T>>& typeArgs, const std::string_view& arg);

		template <class T>
		static T get(const std::vector<Option<T>>& typeArgs, const std::string& name);

		template <class T>
		static void checkValidity(const std::vector<Option<T>>& typeArgs);

	public:
		BasicArgParser(const std::string& programName,
					   const std::vector<BoolArg>&  boolArgs,
					   const std::vector<IntArg>&   intArgs,
					   const std::vector<FloatArg>& floatArgs,
					   const std::vector<TextArg>&  textArgs,
					   bool firstArgumentIsExecutablePath = true,
					   size_t descriptionIndentation = 25);

		void parse(int argc, char const* argv[]);
		void parse(const std::vector<std::string>& args);
		void validate() const;
		void reset();

		inline bool      getBool(const std::string& name) const;
		inline IntType   getInt(const std::string& name) const;
		inline FloatType getFloat(const std::string& name) const;
		inline TextType  getText(const std::string& name) const;

		std::string help() const;
	};

	using ArgParser = BasicArgParser<int, float, std::string>;
		
	
	template <class T>
	Option<T>::
	Option(
		const std::string& name,
		const std::string& description,
		const std::vector<std::string>& arguments,
		const std::optional<T>& defaultValue,
		const std::function<bool(T)>& validityChecker) :
		m_name{name}, m_description{description},
		m_arguments{arguments}, m_validityChecker{validityChecker},
		m_required{!defaultValue.has_value()}, m_defaultValue{defaultValue.has_value() ? *defaultValue : T{}},
		m_value{m_defaultValue} {}
	
	template <class T>
	bool Option<T>::
	operator== (const std::string_view& arg) const {
		if constexpr(std::is_same_v<T, bool>) {
			return std::find(m_arguments.begin(), m_arguments.end(), arg) != m_arguments.end();
		}
		else {
			for (auto&& argument : m_arguments) {
				if (arg.size() >= argument.size() && arg.substr(0, argument.size()) == argument)
					return true;
			}
			return false;
		}
	}
	template <class T>
	void Option<T>::
	operator= (const std::string_view& arg) {
		if (m_alreadySeen)
			throw std::runtime_error("Option \"" + m_name + "\" repeated multiple times: " + std::string{arg});
		m_alreadySeen = true;

		if constexpr(std::is_same_v<bool, T>) {
			m_value = true;
		}
		else {
			std::string argValue;
			for (auto&& argument : m_arguments) {
				if (arg.size() > argument.size() && arg.substr(0, argument.size()) == argument) {
					argValue = arg.substr(argument.size());
					break;
				}
			}
			if (argValue.empty())
				throw std::runtime_error("Option \"" + m_name + "\" requires a value: " + std::string{arg});
			

			if constexpr(std::is_integral_v<T>) {
				try {
					size_t usedCharacters;
					m_value = std::stoll(argValue, &usedCharacters);
					if (usedCharacters != argValue.size())
						throw std::invalid_argument("");
				}
				catch (std::invalid_argument&) {
					throw std::runtime_error("Option \"" + m_name + "\": \"" + argValue + "\" is not an integer: " + std::string{arg});
				}
				catch (std::out_of_range&) {
					throw std::runtime_error("Option \"" + m_name + "\": integer \"" + argValue + "\" is too big and not representable in " + std::to_string(8 * sizeof(long long)) + " bits: " + std::string{arg});
				}
			}
			else if constexpr(std::is_floating_point_v<T>) {
				try {
					size_t usedCharacters;
					m_value = std::stold(argValue, &usedCharacters);
					if (usedCharacters != argValue.size())
						throw std::invalid_argument("");
				}
				catch (std::invalid_argument&) {
					throw std::runtime_error("Option \"" + m_name + "\": \"" + argValue + "\" is not a decimal: " + std::string{arg});
				}
				catch (std::out_of_range&) {
					throw std::runtime_error("Option \"" + m_name + "\": decimal \"" + argValue + "\" is too big and not representable in " + std::to_string(8 * sizeof(long double)) + " bits: " + std::string{arg});
				}
			}
			else {
				m_value = argValue;
			}
		}
	}
	template <class T>
	void Option<T>::
	checkValidity() const {
		if (m_required && !m_alreadySeen)
			throw std::runtime_error("Option \"" + m_name + "\" is required");

		if (!m_validityChecker(m_value)) {
			if constexpr(std::is_same_v<bool, T>)
				throw std::runtime_error("Option \"" + m_name + "\" " + (m_value ? "can't be used" : "is required"));
			else if constexpr(std::is_integral_v<T> || std::is_floating_point_v<T>)
				throw std::runtime_error("Option \"" + m_name + "\": value " + std::to_string(m_value) + " is not allowed");
			else {
				if constexpr(std::is_constructible_v<std::string, T>)
					throw std::runtime_error("Option \"" + m_name + "\": value " + std::string{m_value} + " is not allowed");
				else if constexpr(std::is_assignable_v<std::string&, T>)
					throw std::runtime_error("Option \"" + m_name + "\": value " + (std::string{} = m_value) + " is not allowed");
				else
					throw std::runtime_error("Option \"" + m_name + "\": value not allowed");
			}
		}
	}
	template <class T>
	void Option<T>::
	reset() {
		m_value = m_defaultValue;
		m_alreadySeen = false;
	}

	template <class T>
	std::string Option<T>::
	name() const {
		return m_name;
	}
	template <class T>
	T Option<T>::
	value() const {
		return m_value;
	}

	template <class T>
	std::string Option<T>::
	help(size_t descriptionIndentation) const {
		std::string result = "  ";
		for (auto&& param : m_arguments) {
			result.append(param);
			if constexpr(!std::is_same_v<bool, T>) {
				if constexpr (std::is_integral_v<T>)
					result += 'I'; // integer
				else if constexpr (std::is_floating_point_v<T>)
					result += 'D'; // decimal floating point
				else
					result += 'T'; // text
			}
			result += ' ';
		}

		if (result.size() < descriptionIndentation)
			result.append(std::string(descriptionIndentation - result.size(), ' '));
		else
			result += ' ';
		
		result.append(m_description);
		result += '\n';
		
		return result;
	}



	template <class IntType, class FloatType, class TextType, class Enable>
	template <class T>
	bool BasicArgParser<IntType, FloatType, TextType, Enable>::
	findAssign(std::vector<Option<T>>& typeArgs, const std::string_view& arg) {
		if (auto found = std::find(typeArgs.begin(), typeArgs.end(), arg); found == typeArgs.end()) {
			return false;
		}
		else {
			(*found) = arg;
			return true;
		}
	}

	template <class IntType, class FloatType, class TextType, class Enable>
	template <class T>
	T BasicArgParser<IntType, FloatType, TextType, Enable>::
	get(const std::vector<Option<T>>& typeArgs, const std::string& name) {
		for(auto&& arg : typeArgs) {
			if (arg.name() == name)
				return arg.value();
		}

		if constexpr(std::is_same_v<bool, T>)
			throw std::out_of_range("stypox::BasicArgParser::getBool(): argument " + name + " not found");
		else if constexpr(std::is_same_v<IntType, T>)
			throw std::out_of_range("stypox::BasicArgParser::getInt(): argument " + name + " not found");
		else if constexpr(std::is_same_v<FloatType, T>)
			throw std::out_of_range("stypox::BasicArgParser::getFloat(): argument " + name + " not found");
		else
			throw std::out_of_range("stypox::BasicArgParser::getText(): argument " + name + " not found");
	}

	template <class IntType, class FloatType, class TextType, class Enable>
	template <class T>
	void BasicArgParser<IntType, FloatType, TextType, Enable>::
	checkValidity(const std::vector<Option<T>>& typeArgs) {
		for (auto&& arg : typeArgs)
			arg.checkValidity();
	}

	template <class IntType, class FloatType, class TextType, class Enable>
	BasicArgParser<IntType, FloatType, TextType, Enable>::
	BasicArgParser(
		const std::string& programName,
		const std::vector<BoolArg>&  boolArgs,
		const std::vector<IntArg>&   intArgs,
		const std::vector<FloatArg>& floatArgs,
		const std::vector<TextArg>&  textArgs,
		bool firstArgumentIsExecutablePath,
		size_t descriptionIndentation) :
		m_boolOptions{boolArgs}, m_intOptions{intArgs},
		m_floatOptions{floatArgs}, m_textOptions{textArgs},
		m_programName{programName}, m_executablePath{firstArgumentIsExecutablePath ? std::optional<std::string_view>{""} : std::optional<std::string_view>{}},
		m_descriptionIndentation{descriptionIndentation} {}

	template <class IntType, class FloatType, class TextType, class Enable>
	void BasicArgParser<IntType, FloatType, TextType, Enable>::
	parse(int argc, char const* argv[]) {
		if (m_executablePath.has_value()) { // if the first argument has to be used as executable path
			if (argc < 1)
				throw std::out_of_range("stypox::BasicArgParser::parse(): too few items");
			m_executablePath = argv[0];
		}
		for (int argIt = m_executablePath.has_value(); argIt < argc; ++argIt) {
			std::string_view arg{argv[argIt]};
			if (!(findAssign(m_boolOptions, arg) ||
				  findAssign(m_intOptions, arg) ||
				  findAssign(m_floatOptions, arg) ||
				  findAssign(m_textOptions, arg)))
				throw std::runtime_error("Unknown argument: " + std::string{arg});
		}
	}
	template <class IntType, class FloatType, class TextType, class Enable>
	void BasicArgParser<IntType, FloatType, TextType, Enable>::
	parse(const std::vector<std::string>& args) {
		if (m_executablePath.has_value()) { // if the first argument has to be used as executable path
			if (args.empty())
				throw std::out_of_range("stypox::BasicArgParser::parse(): too few items");
			m_executablePath = args[0];
		}
		for (auto arg = args.begin() + m_executablePath.has_value(); arg != args.end(); ++arg) {
			if (!(findAssign(m_boolOptions, *arg) ||
				  findAssign(m_intOptions, *arg) ||
				  findAssign(m_floatOptions, *arg) ||
				  findAssign(m_textOptions, *arg)))
				throw std::runtime_error("Unknown argument: " + *arg);
		}
	}
	template <class IntType, class FloatType, class TextType, class Enable>
	void BasicArgParser<IntType, FloatType, TextType, Enable>::
	validate() const {
		checkValidity(m_boolOptions);
		checkValidity(m_intOptions);
		checkValidity(m_floatOptions);
		checkValidity(m_textOptions);
	}
	template <class IntType, class FloatType, class TextType, class Enable>
	void BasicArgParser<IntType, FloatType, TextType, Enable>::
	reset() {
		for (auto&& option : m_boolOptions) option.reset();
		for (auto&& option : m_intOptions) option.reset();
		for (auto&& option : m_floatOptions) option.reset();
		for (auto&& option : m_textOptions) option.reset();
	}

	template <class IntType, class FloatType, class TextType, class Enable>
	bool BasicArgParser<IntType, FloatType, TextType, Enable>::
	getBool(const std::string& name) const {
		return get(m_boolOptions, name);
	}	
	template <class IntType, class FloatType, class TextType, class Enable>
	IntType BasicArgParser<IntType, FloatType, TextType, Enable>::
	getInt(const std::string& name) const {
		return get(m_intOptions, name);
	}	
	template <class IntType, class FloatType, class TextType, class Enable>
	FloatType BasicArgParser<IntType, FloatType, TextType, Enable>::
	getFloat(const std::string& name) const {
		return get(m_floatOptions, name);
	}
	template <class IntType, class FloatType, class TextType, class Enable>
	TextType BasicArgParser<IntType, FloatType, TextType, Enable>::
	getText(const std::string& name) const {
		return get(m_textOptions, name);
	}

	template <class IntType, class FloatType, class TextType, class Enable>
	std::string BasicArgParser<IntType, FloatType, TextType, Enable>::
	help() const {
		std::string result = m_programName;
		result.append(": Help screen\n\nUsage: ");
		if (m_executablePath.has_value()) {
			result.append(*m_executablePath);
			result.append(" [OPTIONS...]\n");
		}
		else {
			result.append("[OPTIONS...]\n");
		}

		if (!m_boolOptions.empty()) {
			result.append("\nSwitchable options:\n");
			for (auto&& boolArg : m_boolOptions)
				result.append(boolArg.help(m_descriptionIndentation));
		}

		if (!m_intOptions.empty() || !m_floatOptions.empty() || !m_textOptions.empty()) {
			result.append("\nValue options (I=integer, D=decimal, T=text):\n");
			for (auto&& intArg : m_intOptions)
				result.append(intArg.help(m_descriptionIndentation));
			for (auto&& floatArg : m_floatOptions)
				result.append(floatArg.help(m_descriptionIndentation));
			for (auto&& textArg : m_textOptions)
				result.append(textArg.help(m_descriptionIndentation));
		}
		
		return result;
	}
}

#endif