#ifndef _STYPOX_ARGPARSER_HPP_
#define _STYPOX_ARGPARSER_HPP_

#include <string>
#include <array>
#include <algorithm>
#include <tuple>
#include <optional>
#include <vector>

namespace stypox {
	template<class... Args>
	constexpr std::array<std::string_view, sizeof...(Args)> args(const Args&... list) {
		return {list...};
	}

	template<class T, size_t N>
	class OptionBase {
		bool m_alreadySeen;
		bool m_required;
	protected:
		const std::string_view m_name;
		T& m_output;
		const std::array<std::string_view, N> m_arguments;
		const std::string_view m_help;

		OptionBase(const std::string_view& name,
				T& output,
				const std::array<std::string_view, N>& arguments,
				const std::string_view& help,
				bool required) :
			m_alreadySeen{false}, m_required{required},
			m_name{name}, m_output{output},
			m_arguments{arguments}, m_help{help} {}

		void updateAlreadySeen(const std::string_view& arg) {
			if (m_alreadySeen)
				throw std::runtime_error("Option " + std::string{m_name} + " repeated multiple times: " + std::string{arg});
			m_alreadySeen = true;
		}

		std::string usage(const std::string_view& typeName) const {
			std::string result;
			if constexpr(N >= 1) {
				result += ' ';
				if(!m_required)
					result += '[';

				result.append(m_arguments[0]);
				result.append(typeName);

				if(!m_required)
					result += ']';
			}
			return result;
		}
		std::string help(size_t descriptionIndentation, const std::string_view& typeName) const {
			std::string result = "  ";
			for (auto&& argument : m_arguments) {
				result.append(argument);
				result.append(typeName);
				result += ' ';
			}

			if (result.size() <= descriptionIndentation) {
				result.append(std::string(descriptionIndentation - result.size(), ' '));
			}
			else {
				result += '\n';
				result.append(std::string(descriptionIndentation, ' '));
			}

			if (m_required)
				result += '*';
			result.append(m_help);
			result += '\n';

			return result;
		}
	public:
		// @return true if @param arg is valid
		virtual bool assign(const std::string_view& arg) = 0;
		void reset() {
			m_alreadySeen = false;
		}

		void checkValidity() const {
			if (m_required && !m_alreadySeen)
				throw std::runtime_error("Option " + std::string{m_name} + " is required");
		}

		virtual std::string usage() const = 0;
		virtual std::string help(size_t descriptionIndentation) const = 0;
	};

	template<size_t N, class T = bool>
	class SwitchOption : public OptionBase<T, N> {
		const T m_valueWhenSet;
	public:
	#if __cplusplus <= 201703L && !defined(__cpp_concepts)
		template<typename Dummy = T /* useless, but needed for SFINAE */>
	#endif
		SwitchOption(const std::string_view& name,
					T& output,
					const std::array<std::string_view, N>& arguments,
					const std::string_view& help,
					const T& valueWhenSet = true,
				#if __cplusplus > 201703L || defined(__cpp_concepts)
					bool required = false)
						requires std::is_same_v<T, bool> :
				#else
					typename std::enable_if_t<std::is_same_v<Dummy, bool>, bool> required = false) :
				#endif
			OptionBase<T, N>{name, output, arguments, help, required},
			m_valueWhenSet{valueWhenSet} {}

	#if __cplusplus <= 201703L && !defined(__cpp_concepts)
		template<typename Dummy = T /* useless, but needed for SFINAE */>
	#endif
		SwitchOption(const std::string_view& name,
					T& output,
					const std::array<std::string_view, N>& arguments,
					const std::string_view& help,
					const T& valueWhenSet,
				#if __cplusplus > 201703L || defined(__cpp_concepts)
					bool required = false)
						requires (!std::is_same_v<T, bool>) :
				#else
					typename std::enable_if_t<!std::is_same_v<Dummy, bool>, bool> required = false) :
				#endif
			OptionBase<T, N>{name, output, arguments, help, required},
			m_valueWhenSet{valueWhenSet} {}

		bool assign(const std::string_view& arg) override {
			if (std::find(this->m_arguments.begin(), this->m_arguments.end(), arg) == this->m_arguments.end()) {
				return false;
			}
			else {
				this->updateAlreadySeen(arg);
				this->m_output = m_valueWhenSet;
				return true;
			}
		}

		std::string usage() const override {
			return OptionBase<T, N>::usage("");
		}
		std::string help(size_t descriptionIndentation) const override {
			return OptionBase<T, N>::help(descriptionIndentation, "");
		}
	};

	template<class T, size_t N, class F>
	#if __cplusplus > 201703L || defined(__cpp_concepts)
		requires requires (T t, const F& f, const std::string_view& s) { t = f(s); }
	#endif
	class ManualOption : public OptionBase<T, N> {
		const F m_assignerFunctor;
	public:
		ManualOption(const std::string_view& name,
					T& output,
					const std::array<std::string_view, N>& arguments,
					const std::string_view& help,
					const F& assignerFunctor,
					bool required = false) :
			OptionBase<T, N>{name, output, arguments, help, required},
			m_assignerFunctor{assignerFunctor} {}

		bool assign(const std::string_view& arg) override {
			if (auto found = std::find_if(
				this->m_arguments.begin(),
				this->m_arguments.end(),
				[&arg](const std::string_view& argument) {
					if (arg.size() < argument.size())
						return false;
					else
						return arg.substr(0, argument.size()) == argument;
				});
				found == this->m_arguments.end()) {
				return false;
			}
			else {
				this->updateAlreadySeen(arg);
				this->m_output = m_assignerFunctor(arg.substr(found->size()));
				return true;
			}
		}

		std::string usage() const override {
			return OptionBase<T, N>::usage("S");
		}
		std::string help(size_t descriptionIndentation) const override {
			return OptionBase<T, N>::help(descriptionIndentation, "S");
		}
	};

	template<class T>
	T argumentFromString(const std::string_view& argValue, const std::string_view& argName, const std::string_view& originalArg) {
		if constexpr(std::is_integral_v<T>) {
			try {
				char* endOfUsedCharacters;
				if constexpr(std::is_signed_v<T>) {
					long long result = std::strtoll(argValue.data(), &endOfUsedCharacters, 10);
					if (endOfUsedCharacters != argValue.end())
						throw std::invalid_argument("");
					if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
						throw std::out_of_range("");
					return result;
				}
				else {
					const char* pos = argValue.begin();
					while (pos != argValue.end() && isspace(*pos)) ++pos;
					if (pos != argValue.end() && *pos == '-')
						throw std::out_of_range("");

					unsigned long long result = std::strtoull(pos, &endOfUsedCharacters, 10);
					if (endOfUsedCharacters != argValue.end())
						throw std::invalid_argument("");
					if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
						throw std::out_of_range("");
					return result;
				}
			}
			catch (std::invalid_argument&) {
				throw std::runtime_error("Option " + std::string{argName} + ": \"" + std::string{argValue} +
					"\" is not an integer: " + std::string{originalArg});
			}
			catch (std::out_of_range&) {
				throw std::runtime_error("Option " + std::string{argName} + ": out of range integer \"" + std::string{argValue} +
					"\" (must be between " + std::to_string(std::numeric_limits<T>::min()) + " and " +
					std::to_string(std::numeric_limits<T>::max()) + "): " + std::string{originalArg});
			}
		}

		else if constexpr(std::is_floating_point_v<T>) {
			try {
				char* endOfUsedCharacters;
				long double result = std::strtold(argValue.data(), &endOfUsedCharacters);
				if (endOfUsedCharacters != argValue.end())
					throw std::invalid_argument("");
				if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
					throw std::out_of_range("");
				return result;
			}
			catch (std::invalid_argument&) {
				throw std::runtime_error("Option " + std::string{argName} + ": \"" + std::string{argValue} +
					"\" is not a decimal: " + std::string{originalArg});
			}
			catch (std::out_of_range&) {
				throw std::runtime_error("Option " + std::string{argName} + ": out of range decimal \"" + std::string{argValue} +
					"\" (must be between " + std::to_string(std::numeric_limits<T>::min()) + " and " +
					std::to_string(std::numeric_limits<T>::max()) + "): " + std::string{originalArg});
			}
		}

		else // text
			return T{argValue};
	}

	constexpr auto defaultOptionValidityChecker = [](auto){ return true; };
	template<class T, size_t N, class F = decltype(defaultOptionValidityChecker)>
	#if __cplusplus > 201703L || defined(__cpp_concepts)
		requires requires (bool b, const F& f, const T& s) { b = f(s); }
	#endif
	class Option : public OptionBase<T, N> {
		const F m_validityChecker;

		inline std::string_view typeName() const {
			if constexpr(std::is_integral_v<T>)            return "I";
			else if constexpr(std::is_floating_point_v<T>) return "D";
			else /* T is text */                           return "T";
		}
	public:
		Option(const std::string_view& name,
			T& output,
			const std::array<std::string_view, N>& arguments,
			const std::string_view& help,
			bool required = false,
			const F& validityChecker = defaultOptionValidityChecker) :
			OptionBase<T, N>{name, output, arguments, help, required},
			m_validityChecker{validityChecker} {}

		bool assign(const std::string_view& arg) override {
			if (auto found = std::find_if(
				this->m_arguments.begin(),
				this->m_arguments.end(),
				[&arg](const std::string_view& argument) {
					if (arg.size() < argument.size())
						return false;
					else
						return arg.substr(0, argument.size()) == argument;
				});
				found == this->m_arguments.end()) {
				return false;
			}
			else {
				this->updateAlreadySeen(arg);
				this->m_output = argumentFromString<T>(arg.substr(found->size()), this->m_name, arg);
				return true;
			}
		}

		void checkValidity() const {
			OptionBase<T, N>::checkValidity();

			if (!m_validityChecker(this->m_output)) {
				if constexpr(std::is_integral_v<T> || std::is_floating_point_v<T>)
					throw std::runtime_error("Option " + std::string{this->m_name} + ": value " + std::to_string(this->m_output) + " is not allowed");
				else if constexpr(std::is_constructible_v<std::string, T>)
					throw std::runtime_error("Option " + std::string{this->m_name} + ": value \"" + std::string{this->m_output} + "\" is not allowed");
				else if constexpr(std::is_assignable_v<std::string&, T>)
					throw std::runtime_error("Option " + std::string{this->m_name} + ": value \"" + (std::string{} = this->m_output) + "\" is not allowed");
				else
					throw std::runtime_error("Option " + std::string{this->m_name} + ": value not allowed");
			}
		}

		std::string usage() const override {
			return OptionBase<T, N>::usage(typeName());
		}
		std::string help(size_t descriptionIndentation) const override {
			return OptionBase<T, N>::help(descriptionIndentation, typeName());
		}
	};

	class HelpSection {
		const std::string_view m_title;
	public:
		HelpSection(const std::string_view& title) :
			m_title{title} {}

		std::string help(size_t) const {
			return std::string{m_title} + '\n';
		}
	};

	template<class... Options>
	class ArgParser {
		std::tuple<Options...> m_options;

		const std::string_view m_programName;
		std::optional<std::string> m_executableName;
		const size_t m_descriptionIndentation;

		bool m_doneAssigning;

		template<size_t I = 0>
		inline void assign(const std::string_view& arg) {
			if constexpr(!std::is_same_v<std::tuple_element_t<I, std::tuple<Options...>>, HelpSection>)
				if (!m_doneAssigning)
					m_doneAssigning = std::get<I>(m_options).assign(arg);
			if constexpr(I+1 != sizeof...(Options))
				assign<I+1>(arg);
		}

		template<size_t I = 0>
		inline void checkValidity() const {
			if constexpr(!std::is_same_v<std::tuple_element_t<I, std::tuple<Options...>>, HelpSection>)
				std::get<I>(m_options).checkValidity();
			if constexpr(I+1 != sizeof...(Options))
				checkValidity<I+1>();
		}

		template<size_t I = 0>
		inline void resetOptions() {
			if constexpr(!std::is_same_v<std::tuple_element_t<I, std::tuple<Options...>>, HelpSection>)
				std::get<I>(m_options).reset();
			if constexpr(I+1 != sizeof...(Options))
				resetOptions<I+1>();
		}

		template<size_t I = 0>
		inline std::string optionsHelp() const {
			std::string result = std::get<I>(m_options).help(m_descriptionIndentation);
			if constexpr(I+1 != sizeof...(Options))
				result.append(optionsHelp<I+1>());
			return result;
		}
		template<size_t I = 0>
		inline std::string optionsUsage() const {
			std::string result;
			if constexpr(!std::is_same_v<std::tuple_element_t<I, std::tuple<Options...>>, HelpSection>)
				result = std::get<I>(m_options).usage();
			if constexpr(I+1 != sizeof...(Options))
				result.append(optionsUsage<I+1>());
			return result;
		}

	public:
		ArgParser(std::tuple<Options...> options,
				const std::string_view& programName,
				size_t descriptionIndentation = 25) :
			m_options{options}, m_programName{programName},
			m_executableName{}, m_descriptionIndentation{descriptionIndentation} {}

		template<class Iter>
		#if __cplusplus > 201703L || defined(__cpp_concepts)
			requires std::is_same_v<typename std::iterator_traits<Iter>::value_type, std::string> ||
				std::is_convertible_v<typename std::iterator_traits<Iter>::value_type, std::string_view>
		#endif
		void parse(Iter first, const Iter& last, bool firstArgumentIsExecutablePath) {
			if (firstArgumentIsExecutablePath) {
				if (first == last)
					throw std::out_of_range("stypox::ArgParser::parse(): too few items");
				m_executableName = *first;
				++first;
			}
			else {
				m_executableName = std::nullopt;
			}

			for(; first != last; ++first) {
				m_doneAssigning = false;
				if constexpr(std::is_same_v<typename std::iterator_traits<Iter>::value_type, std::string>) {
					assign(first->c_str());
					if(!m_doneAssigning)
						throw std::runtime_error("Unknown argument: " + *first);
				}
				else {
					assign(*first);
					if(!m_doneAssigning)
						throw std::runtime_error("Unknown argument: " + std::string{std::string_view{*first}});
				}
			}
		}
		void parse(int argc, char const* argv[], bool firstArgumentIsExecutablePath = true) {
			return parse(argv, argv+argc, firstArgumentIsExecutablePath);
		}

		template<class Iter>
		#if __cplusplus > 201703L || defined(__cpp_concepts)
			requires std::is_same_v<typename std::iterator_traits<Iter>::value_type, std::string> ||
				std::is_convertible_v<typename std::iterator_traits<Iter>::value_type, std::string_view>
		#endif
		std::vector<std::string> parsePositional(Iter first, const Iter& last, bool firstArgumentIsExecutablePath) {
			if (firstArgumentIsExecutablePath) {
				if (first == last)
					throw std::out_of_range("stypox::ArgParser::parse(): too few items");
				m_executableName = *first;
				++first;
			}
			else {
				m_executableName = std::nullopt;
			}

			std::vector<std::string> positionalArguments;
			for(; first != last; ++first) {
				m_doneAssigning = false;
				if constexpr(std::is_same_v<typename std::iterator_traits<Iter>::value_type, std::string>) {
					assign(first->c_str());
					if(!m_doneAssigning)
						positionalArguments.push_back(first);
				}
				else {
					assign(*first);
					if(!m_doneAssigning)
						positionalArguments.push_back(std::string{std::string_view{*first}});
				}
			}

			return positionalArguments;
		}
		std::vector<std::string> parsePositional(int argc, char const* argv[], bool firstArgumentIsExecutablePath = true) {
			return parsePositional(argv, argv+argc, firstArgumentIsExecutablePath);
		}

		void validate() const {
			checkValidity();
		}

		void reset() {
			m_executableName = std::nullopt;
			resetOptions();
		}

		std::string usage() const {
			std::string result{m_programName};
			result.append("\nLegend: I=integer; D=decimal; T=text; S=custom string; *=required;\nUsage:");
			if (m_executableName.has_value()) {
				result += ' ';
				result.append(*m_executableName);
			}

			result.append(optionsUsage());
			result += '\n';
			return result;
		}
		std::string help() const {
			std::string result = usage();
			result.append(optionsHelp());
			result += '\n';
			return result;
		}
	};
}

#endif
