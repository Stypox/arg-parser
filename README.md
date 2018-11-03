# Arguments parser

A **C++** library that **parses command line arguments** and automatically generates a **help screen**. It will be compile-time initialized when C++20 will come.

# Features

### **Options**
There are four types of options that can be parsed:
 - **Boolean**: they aren't followed by any value and evaluate to `false`, by default, when they are not specified and to `true` when they are.
 - **Integer**: they except an integer value. The maximum size is the same as `long long`. In the help screen they are represented by an I.
 - **Decimal**: they except an decimal value. The maximum size is the same as `long double`. In the help screen they are represented by a D.
 - **Text**: they except a string of text. In the help screen they are represented by a T.

Every option has:
 - a ***name***: used to get the option value after parsing and for error reporting;
 - a ***description***: used in the help screen;
 - one or more ***parameters***: what the user has to pass in the console to set the value of the option;
 - a ***default value***: when not set, the user is required to provide a value for the option via command line;
 - a ***validity checker*** function: used to determine whether a value is valid or not;

### **Error checking and reporting**
During the parsing process every argument has to meet these requirements:
 - it must have a **corresponding option**;
 - that option should **not have been encountered** already;
 - the argument has to **contain a value**, but only if the type of the option is not boolean;
 - the value has to be of the **same type** of the option;
 - the value can't **overflow**;

During the validation process every computed option has to meet these requirements:
 - if the option is required, it must **have been encountered**;
 - the value must **pass the validity check**, represented by the provided function (that returns true by default);

The parsing process and the validation process are **separate**, so that even if an option is invalid no error is thrown until the validation starts. This is useful, for example, to display the help screen when `--help` is provided, even if other options are invalid. Every error contains a **detailed description** about what caused it.

### **Other**
 - The intentation of the description of sections can be changed.
 - The first argument is considered, by default, the executable path, but this can be manually changed.

# Installation
Just **download** the header file inside the subfolder src/ and **`#include`** it into your project! Note: it requires C++17, so add to your compiler options `-std=c++17`.

# Documentation
All types are defined in `namespace stypox`
<br>
<br>
`BasicArgParser` is the class that does the job of parsing arguments. A type can be chosen manually for integer, floating-point and text options. The preconfigured `ArgParser` is an alias for `BasicArgParser<int, float, std::string>`.

### **BasicArgParser::BasicArgParser()**
Takes, in order, 7 arguments: the program name, a list of boolean options, a list of integer options, a list of decimal options, a list of text options, a boolean representing whether the first argument has to be considered the executable path (defaults to 25), the size of the indentation.  
Constructs the BasicArgParser object. It will be constexpr in C++20.

### **BasicArgParser::parse()**
Takes 2 arguments: the number of arguments and an array of pointers to char.  
Parses all the arguments, reports parsing errors (by throwing std::runtime_error) as described above, saves the new values for options.

### **BasicArgParser::validate()**
Takes no argument.  
Reports logical errors (by throwing std::runtime_error) as described above.

### **BasicArgParser::getBool(), getInt(), getFloat(), getText()**
They take 1 argument: the name of the option to look for.  
Returns the value of that option; throws std::out_of_range.

### **BasicArgParser::help()**
Takes no argument.  
Returns a string containing the help screen. The indentation of the description of options can be set in the constructor. It will be constexpr in C++20. It is formatted this way (see example below):
```
Example program: Help screen

Usage: ./arg-parser.exe [OPTIONS...]

Switchable options:
  -? -h -H --help                       shows this screen
  --hello --hey                         greetings!

Value options (I=integer, D=decimal, T=text):
  -e=I --exit=I --exit-code=I           exit code (required)
  -c=D --cake=D                         how large should the piece of cake be? (range 0-1, default 0)
  -p=T --print=T                        prints to the console
```
<br>
<br>
``Option`` is the class that keeps information about every option.

### **Option::Option()**
Takes 5 arguments: the name, the description, a list of possible arguments, the default value (defaults to the default value of the type and a validity checker function (defaults to a function that always returns true). The default value is of type `std::optional`, so if there is no default value (i.e. the option is required) it can be set to `{}` (= no default value).  
Constructs the Option object. It will be constexpr in C++20.



# Example
```cpp
int main(int argc, char const *argv[]) {
ArgParser p{
	// program name
	"Example program",
	// list of boolean options
	{
		{"help", "shows this screen", {"-?", "-h", "-H", "--help"}},
		{"hello", "greetings!", {"--hello", "--hey"}}
	},
	// list of integer options
	{
		//here {} indicates that the default value does not exist, i.e the option is required
		{"exit", "exit code (required)", {"-e=", "--exit=", "--exit-code="}, {}}
	},
	// list of decimal options
	{
		{
			// name
			"cake",
			// description
			"how large should the piece of cake be? (range 0-1, default 0)",
			// list of possible arguments
			{"-c=", "--cake="},
			// default value (defaults to the default value of the type)
			0.0f,
			// validity checker function
			[](float f){ return f >= 0.0f && f <= 1.0f; }
		}
	},
	// list of text options
	{
		{"print", "prints to the console", {"-p=", "--print="}, "nothing to print :-("}
	},
	// has the first argument to be considered the executable path? (defaults to true)
	true,
	// indentation (defaults to 25)
	40
};
p.parse(argc, argv); // parse arguments

if (p.getBool("help")) {
	std::cout << p.help();
	exit(0);
}
p.validate(); // validate options after checking for help

if (p.getBool("hello"))
	std::cout << "Hello! :-)\n";
std::cout << "Taking a piece of cake of size " << std::to_string(p.getFloat("cake")) << " :-D\n";
std::cout << p.getText("print") << "\n";

return p.getInt("exit");
}
```
Output for different commands:
```
$ ./arg-parser.exe -e=508 --hello -c=0.000001 --print="good evening"
Hello! :-)
Taking a piece of cake of size 0.000001 :-D
good evening

$ ./arg-parser.exe -c=-10
terminate called after throwing an instance of 'std::runtime_error'
  what():  Argument "exit" is required
Aborted

$./arg-parser.exe -c=-10 -e=0
terminate called after throwing an instance of 'std::runtime_error'
  what():  Argument "cake": value -10.000000 is not allowed
Aborted

$ ./arg-parser.exe -e=
terminate called after throwing an instance of 'std::runtime_error'
  what():  Argument "exit" requires a value: -e=
Aborted

$ ./arg-parser.exe -e=0
Taking a piece of cake of size 0.000000 :-D
nothing to print :-(
```