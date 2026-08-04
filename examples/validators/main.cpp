// validator feature
// you can chain validators to anything but a flag. The validator's signature
// is a function that takes the parsed value and returns:
// - an empty string on success
// - an error message on failure
//
// CLAP offers (as of Jul 31 2026, pre 1.0.0) the following 8 builtins:
// - Range(lo, hi) accepts [lo, hi]. needs <= and <<.
// - Choices(vals) accepts only listed values. needs == and <<.
// - Min(min) accepts [min, +inf). needs <= and <<.
// - Max(max) accepts [-inf, max]. needs <= and <<.
// - FileExists accepts only existing files.
// - DirExists accepts only existing directories.
// - NonexistentPath accepts only non-existing paths.
// - NonEmpty accepts only non-empty values. needs empty().
// 
// Call them like so, inside the .validator() method:
//    clap::Range(0, 10)
// 
// (see example below)
//
// You can also easily make your own validation function, as a lambda or
// callable object. If you really want, you can also create your object with
// .label() and/or .hint() members, and CLAP will use them in the help.
// A label is displayed as part of the type slot, a hint as a parenthesis after
// the description:
//
//     $ ./examples/validators/validators -h
//     Usage: prog [-h] [-r <int 0..10>] [--min <int>]
//                               🠕
//                   this is the label
//     Options:           🠗
//       -r,--range  <int 0..10>  showcase the range validator
//       --min       <int>        showcase the min validator (>= 1)
//                                                             🠕
//                                                    this is the hint
// 
// The example below shows all builtins. It is obviously not practical and you
// would never use all of them in a single program.
// 
// the prior "validators" were the .range() and .choices() methods, which now
// wrap their respective validators (as mentionned above). They are still
// available, kept as syntactic sugar. I don't recommand using them, as i might
// remove them entirely in the future, making them deprecated. 

#include "clap.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    clap::App app(argv[0], "showcase the validator feature");
    app.footer(
        "this program is a showcase of the validator feature. It shows\n"
        "all the builtin validators, plus a custom one. It is obviously\n"
        "not practical and you would never use all of them at once."
    );

    auto& help = app.flag("-h,--help", "show this help and exit");
    auto& range = app.option<int>("-r,--range", "showcase the range validator")
                     .validator(clap::Range(0, 10));

    auto& min = app.option<int>("--min", "showcase the min validator")
                   .validator(clap::Min(1));

    auto& max = app.option<int>("--max", "showcase the max validator")
                   .validator(clap::Max(10));

    auto& choices = app.option<int>("-c,--choices", "showcase the choices validator")
                       .validator(clap::Choices({1, 2, 3}));

    auto& str = app.option<std::string>("--string", "showcase non empty validator")
                  .validator(clap::NonEmpty);

    using path = std::filesystem::path;

    auto& file = app.option<path>("-f,--file", "showcase file exists validator")
                   .validator(clap::FileExists);

    auto& nex_file = app.option<path>("-o,--output", "showcase file exists validator")
                                 .validator(clap::NonexistentPath);

    auto& dir = app.option<path>("-d,--directory", "showcase dir exists validator")
                  .validator(clap::DirExists);

    auto& ne_path = app.option<path>("-n,--non-empty-path", "showcase non existing path validator")
                     .validator(clap::NonEmpty);

    auto& p2 = app.option<int>("-p,--power-or-two", "showcase custom validator lambda")
                  .validator([](int v) -> std::string {
                        if (v <= 0) return "must be positive";
                        if ((v & (v - 1)) != 0) return "must be a power of two";
                        return "";
                  });

    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    if (range.is_set())    std::cout << "range: " << range.get() << "\n";
    if (min.is_set())      std::cout << "min: " << min.get() << "\n";
    if (max.is_set())      std::cout << "max: " << max.get() << "\n";
    if (choices.is_set())  std::cout << "choices: " << choices.get() << "\n";
    if (str.is_set())      std::cout << "string: " << str.get() << "\n";
    if (file.is_set())     std::cout << "file: " << file.get() << "\n";
    if (nex_file.is_set()) std::cout << "non-existing file: " << nex_file.get() << "\n";
    if (dir.is_set())      std::cout << "dir: " << dir.get() << "\n";
    if (ne_path.is_set())  std::cout << "non-empty path: " << ne_path.get() << "\n";
    if (p2.is_set())       std::cout << "power of two: " << p2.get() << "\n";
}
