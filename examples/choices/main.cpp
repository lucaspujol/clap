#define CLAP_IMPLEMENTATION
#include "clap.hpp"

#include <iostream>
#include <string>

// imagine a formatter of some sort, where you pick the 
// format you want to output to: json, yaml or xml
int main(int argc, char **argv) {
    clap::App app(argv[0], "showcase of choices feature.");

    auto &help = app.flag("-h,--help", "show this help message");
    auto &format = app.option<std::string>("-f,--format", "the file format chosen.")
                      .choices({"json", "yaml", "xml"})         // pick the choices available for the user
                      .required();
    
    bool ok = app.parse(argc, argv);
    if (help) { std::cout << app.help();  return 0; }
    if (!ok)  { std::cerr << app.error(); return 1; }

    std::string file_format = format.get();
    // if (file_format != "json" &&
    //     file_format != "yaml" &&
    //     file_format != "xml"
    // ) {
    //     std::cerr << "invalid file format" << std::endl;
    //     return 1;
    // }
    std::cout << "file format is: " << file_format << std::endl;
}
