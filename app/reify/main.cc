// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#include "reify/program.hh"
#include <fstream>
#include <potassco/application.h>
#include <potassco/program_opts/typed_value.h>
#include <potassco/error.h>

using namespace Potassco::ProgramOptions;

struct ReifyOptions {
    bool calculateSCCs = false;
    bool reifyStep = false;
};

class ReifyApp : public Potassco::Application {
  public:
    [[nodiscard]] std::string_view getName() const override { return "reify"; }
    [[nodiscard]] std::string_view getVersion() const override { return "2.0.0"; }
    [[nodiscard]] std::string_view getPositional(std::string_view) const override { return "input"; }
    [[nodiscard]] std::string_view getUsage() const override {
        return "[options] [<file>]\n"
               "Convert program in <file> or standard input";
    }
    void initOptions(OptionContext& root) override {
        OptionGroup reify("Reify Options");
        reify.addOptions()
            ("-i@2,input", storeTo(input_, std::string()), "Input file")
            ("-c, sccs", flag(opts_.calculateSCCs), "calculate strongly connected components")
            ("-s, steps", flag(opts_.reifyStep), "add step numbers to generated facts")
            ;
        root.add(std::move(reify));
    }

    void validateOptions(const OptionContext&, const ParsedOptions& parsed) override {}
    void setup() override {}
    void onHelp(const std::string& info, Potassco::ProgramOptions::DescriptionLevel) override {
        std::cout << info << "\n";
    }
    void onVersion(const std::string& info) override {
        std::cout << info << "\nlibpotassco version " << LIB_POTASSCO_VERSION
                  << "\nCopyright (C) Benjamin Kaufmann\n"
                     "License: The MIT License <https://opensource.org/licenses/MIT>\n";
    }
    void run() override {
        Reify::Reifier reify(std::cout, opts_.calculateSCCs, opts_.reifyStep);
        std::ifstream iFile;
        if (not input_.empty() && input_ != "-") {
            iFile.open(input_.c_str());
            POTASSCO_CHECK(iFile.is_open(), std::errc::no_such_file_or_directory, "Could not open input file");
        }
        std::istream& in = iFile.is_open() ? iFile : std::cin;
        reify.parse(in);
    }
    bool onUnhandledException(const std::exception_ptr&, std::string_view msg) noexcept override {
        std::cerr << msg << "\n";
        return false;
    }
    void flush() override {
        std::cout.flush();
        std::cerr.flush();
    }
  private:
    std::string input_;
    ReifyOptions opts_;
};

int main(int argc, char **argv) {
    ReifyApp app;
    return app.main(argc, argv);
}
