# Argument parser

A **C++** library that **parses command line arguments** and automatically generates a **help screen**. The initialization of the parser should not impose **any run-time overhead**.

# Features

## **Options** 
There are three different options that can be used:
 - **Switch option**: they are not followed by any value (e.g. `--help`). When used they set the underlying reference to a provided value. 
 - **Option**: they except an integer, a decimal number or some text (e.g. `--size=50 --gravity=9.8 --say=Hello!`).
   - The type `T` of the underlying reference must meet one of these requirements:
     - `std::is_integer<T>`: excepts an integer that does not overflow/underflow `T` limits.
	  Represented by `I` in the help screen.
     - `std::is_floating_point<T>`: excepts a decimal number that does not overflow/underflow `T` limits.
	  Represented by `D` in the help screen.
     - `std::is_convertible<std::string_view, T>`: excepts some text.
	  Represented by `T` in the help screen.
	- excepts a valid value, checked using a **user-defined validation function** (if present).
 - **Manual option**: they except an arbitrary string which is manipulated in a non-standard way. (e.g. `--html=<p>Hello!</p>`)
	- Requires a **user-defined function** to convert the string to the type of the underlying reference.
	- Represented by `S` in the help screen.
   

Every option has these attributes:
 - ***name***: used for error reporting;
 - ***reference*** to a variable: the output in which to save the inserted value;
 - one or more ***arguments***: what the user has to pass in the console to set the value of the option;
 - ***description***: used in the help screen;
 - whether it is ***required*** or not; represented by `*` in the help screen when set to `true`;

## **Error checking and reporting**
During the parsing process every argument has to meet these requirements:
 - every argument must have a **corresponding option** (this does not apply if positional arguments are valid);
 - that option should **not have already been encountered**;
 - if the option excepts a **value**, the argument must contain one (but empty texts/strings are ok);
 - the value has to be **convertible** to the underlying variable type (either normally or via the user-defined function);
 - for *normal options*, the value must **not overflow** (this only applies to integers and decimal numbers)

During the validation process every computed option has to meet these requirements:
 - if the option is required, it must **have been encountered**;
 - for *normal options*, the value must **pass the validity check**, represented by the user-defined function (that passes by default);

The parsing process and the validation process are **separate**, so that even if an option is invalid no error is generated until the validation starts. This is useful, for example, to display the help screen when `--help` is provided, even if other options are invalid. Every error contains an **thorough description** about what caused it.

## **Help screen**
See [below](#output) for an example help screen.
 - Titles and lines of description can be added to the help screen by providing **help sections**.
 - The **intentation** of the description of sections can be changed. When the indentation is not enough a newline is added between the arguments and the description
 - The first argument is considered, by default, the **executable path**, but this can be manually changed. The executable path is used for the help screen.

# Installation
Just **download** the header file `argparser.h` and **`#include`** it into your project! If you want to `#include` it as `<stypox/argparser.h>` you need to add `-IPATH/TO/arg-parser/include` to your compiler options.  
Note: it requires C++17, so add to your compiler options `-std=c++17`. C++20 is also supported, along with `requires` clauses.

# Documentation
All types are defined in `namespace stypox`. To simplify reading, `std::` is omitted before `tuple`, `string`, `string_view`, `vector` and `array`; also `const Type&` is written just `Type`. Read the code for more precise details.
## **ArgParser**
`ArgParser` is the class that does the job of parsing arguments.

### ArgParser::ArgParser()
`(tuple<Options...> options, string_view programName, size_t descriptionIndentation = 25)`  
Constructs the ArgParser object. `Options...` must be made only of `SwitchOption`, `Option`, `ManualOption` or `HelpSection`. The `tuple` can be instantiated using `std::make_tuple(Options...)`.

### ArgParser::parse()
(1) `void (Iter first, Iter last, bool firstArgumentIsExecutablePath)`  
(2) `void (int argc, char const* argv[], bool firstArgumentIsExecutablePath = true)`  
Parses all the arguments in range [first, last) (1) / [argv, argv+argc) (2), reports parsing errors (by throwing `std::runtime_error`) as described [above](#options), saves the new values for options. Throws `std::out_of_range` if `firstArgumentIsExecutablePath` is set to `true` but the list of arguments is empty.

### ArgParser::parsePositional()
(1) `vector<string> (Iter first, Iter last, bool firstArgumentIsExecutablePath)`  
(2) `vector<string> (int argc, char const* argv[], bool firstArgumentIsExecutablePath = true)`  
Parses all the arguments in range [first, last) (1) / [argv, argv+argc) (2), reports parsing errors (by throwing `std::runtime_error`) as described [above](#options), saves the new values for options. Returns the arguments that didn't match any option. Throws `std::out_of_range` if `firstArgumentIsExecutablePath` is set to `true` but the list of arguments is empty.

### ArgParser::validate()
`void ()`  
Reports logical errors (by throwing `std::runtime_error`) as described [above](#error-checking-and-reporting).

### ArgParser::reset()
`void ()`  
Every argument is set as if it had never been encountered.

### ArgParser::usage()
`string ()`  
Returns the usage screen. See the [output of the code below](#output) for an example.

### ArgParser::help()
`string ()`  
Returns the help screen. The indentation of the description of options can be set in the constructor. See the [output of the code below](#output) for an example.

## SwitchOption, Option, ManualOption, HelpSection
`HelpSection`
`SwitchOption`, `Option` and `ManualOption` are the classes that keep information about every option. The difference between them is explained [above](#options). The array of possible arguments (of size `N`) can be initialized using `stypox::args()`.  
`HelpSection` is a class that holds a string of text to be printed in the help screen.

### args()
`array<string_view, sizeof...(Args)> args(Args... list)`  
Builds an array of possible arguments using the provided `list` (needed for the constructors of options).

### SwitchOption::SwitchOption()
(when T is bool) `(string_view name, T& output, array<string_view, N> arguments, string_view help, T valueWhenSet = true, required = false)`  
(when T is not bool) `(string_view name, T& output, array<string_view, N> arguments, string_view help, T valueWhenSet, required = false)`  
`SwitchOption`'s constructor. When parsing, the value will be saved in `output`.

### Option::Option()
`(string_view name, T& output, array<string_view, N> arguments, string_view help, required = false, F validityChecker = [](){ return true; })`  
`Option`'s constructor. When parsing, the value will be saved in `output`. When validating `validityChecker` is called with `(output)` (it must return `bool`). See [above](#options) to read about the valid types `T`.

### ManualOption::ManualOption()
`(string_view name, T& output, array<string_view, N> arguments, string_view help, F assignerFunctor, required = false)`  
`Option`'s constructor. When parsing, the value, converted to `T` by calling `assignerFunctor(string_view)`, will be saved in `output`.

### HelpSection::HelpSection()
`(string_view title)`  
`HelpSection`'s constructor. When generating the help screen `title` is appended to it followed by `\n`.


# Example
```cpp#include <iostream>
#include "arg-parser/include/stypox/argparser.h"

int main(int argc, char const* argv[]) {
	// initialize variables with the default values
	bool help = false;
	bool usage = false;
	int exitCode = 0;
	float cake = 0.5f;

	// a default value would be useless, since the corresponding option is required
	std::pair<std::string, int> person;

	stypox::ArgParser p{
		std::make_tuple(
			stypox::HelpSection{"\nHelp options:"},
			stypox::SwitchOption{"help", help, stypox::args("-?", "-h", "--help"), "show help screen"},
			stypox::SwitchOption{"usage", usage, stypox::args("-u", "--usage"), "show usage screen"},

			stypox::HelpSection{"\nOther important options:"},
			stypox::SwitchOption{"exitWithError", exitCode, stypox::args("--exit-with-error"), "set exit code to 1",
				1 /* value to set to variable @exitCode when the option is used*/},
			stypox::Option{"cake", cake, stypox::args("-c=", "--cake="), "how much cake will you take? (0-1; default=0.5)",
				false, // option not required
				[](float value){ // validation function
					return value >= 0.0f && value <= 1.0f;
				}},
			stypox::ManualOption{"person", person, stypox::args("-p=", "--person="), "a person (format: `name;age`)",
				[](const std::string_view& str) { // conversion function
					try {
						size_t semicolonIndex = str.find_first_of(";");
						if (semicolonIndex == std::string::npos)
							throw std::runtime_error{""};

						std::string name{str.substr(0, semicolonIndex)},
							age{str.substr(semicolonIndex + 1, str.size() - semicolonIndex - 1)};
						// the type of the returned value matches the type of variable @person
						return std::pair<std::string, float>{name, std::stoi(age)};
					} catch (...) {
						throw std::runtime_error{"Invalid person " + std::string{str}};
					}
				},
				true /* option required */}
		),
		"Example program by Stypox"
	};

	p.parse(argc, argv);
	// check for help before validating
	if (help) {
		std::cout << p.help();
		return 0;
	}
	if (usage) {
		std::cout << p.usage();
		return 0;
	}

	p.validate();
	std::cout << "Here is a piece of cake of size " << cake << "!\n"
		<< person.first << " is now my best friend: he is " << person.second << " years old :-D\n";

	return exitCode;
}
```
### Output
Output for different commands (`; echo 'Exit code: '$?;` is only there to print the exit code):
```
$ ./executable; echo 'Exit code: '$?;
terminate called after throwing an instance of 'std::runtime_error'
  what():  Option person is required
Aborted
Exit code: 134


$ ./executable --help; echo 'Exit code: '$?;
Example program by Stypox
Legend: I=integer; D=decimal; T=text; S=custom string; *=required;
Usage: ./executable [-?] [-u] [--exit-with-error] [-c=D] -p=S

Help options:
  -? -h --help           show help screen
  -u --usage             show usage screen

Other important options:
  --exit-with-error      set exit code to 1
  -c=D --cake=D          how much cake will you take? (0-1; default=0.5)
  -p=S --person=S        *a person (format: `name;age`)

Exit code: 0


$./executable --usage; echo 'Exit code: '$?;
Example program by Stypox
Legend: I=integer; D=decimal; T=text; S=custom string; *=required;
Usage: ./executable [-?] [-u] [--exit-with-error] [-c=D] -p=S
Exit code: 0


$./executable --cake=0.7245 --person="John Smith;17" --exit-with-error; echo 'Exit code: '$?;
Here is a piece of cake of size 0.7245!
John Smith is now my best friend: he is 17 years old :-D
Exit code: 1


$ ./executable --cak; echo 'Exit code: '$?;
terminate called after throwing an instance of 'std::runtime_error'
  what():  Unknown argument: --cak
Aborted
Exit code: 134


$ ./executable --cake=1.2; echo 'Exit code: '$?;
terminate called after throwing an instance of 'std::runtime_error'
  what():  Option cake: value 1.200000 is not allowed
Aborted
Exit code: 134


$ ./executable --cake=1.2e10000; echo 'Exit code: '$?;
terminate called after throwing an instance of 'std::runtime_error'
  what():  Option cake: out of range decimal "1.2e10000" (must be between 0.000000 and 340282346638528859811704183484516925440.000000): --cake=1.2e10000
Aborted
Exit code: 134
```
