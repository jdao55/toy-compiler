#ifndef __ARGPARSER_H_
#define __ARGPARSER_H_

#include <docopt/docopt.h>
#include <fmt/format.h>
#include <variant>
#include <stdexcept>


struct Arguments
{
    std::string srcfilename;
    std::string outfilename = "output.o";
    int8_t opt_level;
};

const char USAGE[] =
    R"(toy compiler
    Usage:
      toycomp <filename> [--out=filename] [--opt=level]
      toycomp (-h | --help)

    Options:
      -h --help                         Show this screen.
      -o filname --out=filename       Specify output object file name
      -O level --opt=level            Specify optimization level [1,2,3])";

inline auto get_args_map(int argc, char **argv)
{
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        { argv + 1, argv + argc },
        true,// show help if requested
        "toycomp 0.1");// version string
    return args;
}


inline std::variant<Arguments, std::string> get_args(int argc, char **argv)
{
    size_t arg_position = 0;
    try
    {
        Arguments args;
        auto args_map = get_args_map(argc, argv);

        if (args_map["--out"])
        {
            auto filter_str = args_map["--out"].asString();
            args.outfilename = filter_str;
            arg_position++;
        }

        if (args_map["<filename>"])
        {
            args.srcfilename = args_map["<filename>"].asString();
            arg_position++;
        }


        if (args_map["--opt"])
        {
            args.opt_level = std::stoi(args_map["--opt"].asString());
            arg_position++;
        }
        return std::variant<Arguments, std::string>(args);
    } catch (const std::invalid_argument &e)
    {
        return fmt::format("Error: invalid argument \"\x1b[38;2;225;100;40m{}\x1b[0m\"\n", argv[arg_position]);
    }
}


#endif// __ARGPARSER_H_
