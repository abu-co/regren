#include <regex>
#include <string>
#include <iostream>
#include <filesystem>
#include <string_view>

#include <gsl/gsl>

int main(int argc, const char* argv[]);

static void output_help_str(std::ostream& output, std::string_view exe_name) {
	output 
		<< exe_name << " <directory_path> <match_pattern> <new_name> [flags]"
		<< R"HELPSTR(

  Where:

    <directory_path> is the directory in which the
      search for matching files with take place;

    <match_pattern> is the regex search pattern
      that matches the files that are to be renamed;

    <new_name> is the pattern that the filenames 
      will be replaced into. 

      The program will terminate if there is already 
      a file with the same name.

  Flags (can be specified separately or in groups): 
    -i: ignore cases.
    -d: match directories and exclude files.
    -p: preview changes; none will actually be made.

  The syntax of the regex is roughly the same as the
  one used by ECMAScript.

  Arguments must be passed in the specified order
  because the developer is lazy.

  Example: 
    )HELPSTR" << exe_name << R"HELPSTR( . 123(.+)321(\.?.*) "$$file_$1$2" -pi -d

    Which will rename "123abc321.a" to "$file_abc.a" 
    and "1233321" to "$file_3".
)HELPSTR";
}

struct Options {
	std::regex match_pattern;
	std::filesystem::path directory;
	std::string new_name;
	bool // Flag...
#ifdef _DEBUG
		is_test = false,
#endif // _DEBUG
		is_directory_only = false, 
		is_preview = false;

	bool test_entry(const std::filesystem::directory_entry& entry) {
		// Mutually excusive test pairs
		const std::pair<bool, bool> tests[] = {
			{ is_directory_only, entry.is_directory() },
			{ !is_directory_only, entry.is_regular_file() },
		};
		for (auto& test_pair : tests) {
			if (test_pair.first && test_pair.second)
				return true;
		}
		return false;
	}
};

static void print_help(std::string_view self_name) {
	const auto exe_name_pos = self_name.find_last_of("/\\") + 1;
	const std::string_view exe_name =
		self_name.substr(exe_name_pos, self_name.length() - exe_name_pos);

	output_help_str(std::cout, exe_name);
}

int parse_args(const gsl::span<const char*>& args, Options& options)
{
	using namespace std::string_literals;
	using namespace std::string_view_literals;
	namespace fs = std::filesystem;
	using std::cerr, std::endl;

	static constexpr const std::string_view help_args[] = { 
		"--help", "-help", "-h", "-?", "/?", "/help"
	};

	static const auto print_arg_err = [](const std::string& message) {
		std::cerr
			<< message << endl
			<< "Use --help for usage information." << endl;
	};

	if (args.size() <= 1 || 
		std::find(
			std::begin(help_args), std::end(help_args), args[1]
		) != std::end(help_args)) {
		print_help(
			(args.empty() || std::strlen(args[0]) == 0) ? "regren" : args[0]
		);
		return 1;
	}
#ifdef _DEBUG
	else if (args[1] == "--test"sv) {
		options.is_test = true;
		return 0;
	}
#endif // _DEBUG
	else if (args.size() <= 3) {
		print_arg_err("Expected 3 arguments.");
		return -1;
	}

	options.directory = fs::path(args[1]);
	std::error_code file_err;

	if (!fs::is_directory(options.directory, file_err)) {
		cerr << "Error: ";
		if (file_err) {
			cerr << file_err << endl;
		}
		else {
			cerr
				<< options.directory
				<< " is not a directory!"
				<< endl;
		}
		return file_err.value();
	}

	file_err.clear();

	auto regex_flags = std::regex_constants::ECMAScript
		| std::regex_constants::optimize;

	for (size_t i = 4; i < args.size(); i++) {
		std::string_view arg = args[i];
		if (arg.size() <= 1 || arg[0] != '-') {
			print_arg_err("Invalid argument \""s + arg[0] + "\": flag(s) expected.");
			return -1;
		}
		for (size_t j = 1; j < arg.size(); j++) {
			switch (arg[j])
			{
			case 'i':
				regex_flags |= std::regex_constants::icase;
				break;
			case 'd':
				options.is_directory_only = true;
				break;
			case 'p':
				options.is_preview = true;
				break;
			default:
				print_arg_err("Unknown flag \"-"s + arg[j] + '\"');
				return -1;
			}
		}
	}

	try {
		options.match_pattern = std::regex(args[2], regex_flags);
	}
	catch (const std::regex_error& e) {
		cerr << "RegExp Error(" << e.code() << "): " << e.what() << endl;
		return -2;
	}

	options.new_name = args[3];

	return 0;
}

#ifdef _DEBUG
int test() {
	using test_t = bool (*)();
	using args_t = gsl::span<const char*>;

	using namespace std::string_literals;
	using namespace std::string_view_literals;

	static constexpr char TEST_ARG0[] = "<test:argv[0]>";

	constexpr test_t tests[]{
		[]() {
			Options o;
			const char new_name[] = "$$$1$$";
			const char* args[] = { TEST_ARG0, ".", "^(.*)\\.@name", new_name };
			int r = parse_args(args, o);
			const auto result = std::regex_replace(
				"f_I+l%e.@name!", o.match_pattern, o.new_name
			);
			const char expected[] = "$f_I+l%e$!";
			return r == 0
				&& o.directory == "."
				&& o.new_name == new_name
				&& result == expected;
		},
		[]() {
			Options o;
			const char* args[] = { TEST_ARG0, ".", "#", "@", "-dpi" };
			int r = parse_args(args, o);
			return r == 0
				&& o.is_directory_only
				&& o.is_preview
				&& o.match_pattern.flags() & std::regex_constants::icase;
		},
		[]() {
			const char* args[] = { 
				TEST_ARG0, ".", "([^.]+)(\\.?.*)", "$$$1_renamed$2", "-pi" 
			};
			std::cout << std::endl;
			return main(static_cast<int>(std::size(args)), args) == 0;
		},
	};

	int failed_test_count = 0;
	for (size_t i = 0; i < std::size(tests); i++) {
		std::cout << "Test #" << i << ": ";
		if (!tests[i]()) {
			failed_test_count++;
			std::cout << "Failed.";
		}
		else {
			std::cout << "Passed.";
		}
		std::cout << std::endl;
	}

	std::cout
		<< "Tests: " 
		<< failed_test_count << '/' << std::size(tests) 
		<< " failed." << std::endl;

	return failed_test_count;
}
#endif // _DEBUG (test content)

int main(int argc, const char* argv[])
{
	namespace fs = std::filesystem;

	constexpr char TAB_CHARS[] = "    ";

	Options options{};

	const auto args = gsl::span(argv, argc);
	if (int err = parse_args(args, options))
		return err;

#ifdef _DEBUG
	if (options.is_test) {
		return test();
	}
#endif // _DEBUG

	std::error_code err;
	size_t renamed_count = 0, total_count = 0;

	for (const auto& entry : fs::directory_iterator(options.directory)) {
		using std::cout, std::cerr, std::endl;

		if (!options.test_entry(entry)) {
			continue;
		}

		const std::string old_name = entry.path().filename().string();
		std::string new_name;

		try {
			if (!std::regex_search(old_name, options.match_pattern))
				continue; // skip; does not match
			new_name = std::regex_replace(
				old_name, options.match_pattern, options.new_name
			);
		}
		catch (const std::regex_error& e) {
			std::cerr
				<< "RegExp error: "
				<< e.what()
				<< endl;
			return e.code();
		}

		const fs::path new_path = options.directory.append(new_name);
		if (fs::exists(new_path, err) || err) {
			if (err) {
				cerr
					<< "Unable to check whether " << new_path << " exists: "
					<< endl
					<< TAB_CHARS << err
					<< endl;
				return err.value();
			}
			cout
				<< "File "
				<< std::quoted(new_name)
				<< " already exists; skipping..."
				<< endl;
			continue;
		}

		err.clear();
		if (!options.is_preview) {
			fs::rename(entry.path(), new_path, err);
		}

		if (err) {
			cerr
				<< "Error: Unable to rename file "
				<< std::quoted(old_name)
				<< " to "
				<< std::quoted(new_name) << ':'
				<< endl
				<< TAB_CHARS << err
				<< endl;
		}
		else {
			cout
				<< std::quoted(old_name)
				<< " --> "
				<< std::quoted(new_name)
				<< endl;
			renamed_count++;
		}
		total_count++;
	}

	std::cout << std::endl;
	std::cout << renamed_count << '/' << total_count << " files ";
	if (options.is_preview)
		std::cout << "will be ";
	std::cout << "renamed. " << std::endl;

	return static_cast<int>(total_count - renamed_count);
}

