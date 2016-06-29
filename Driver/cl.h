/*
 * cl.h
 *
 *  Created on: Aug 26, 2013
 *      Author: belyaev
 */

#ifndef CL_H_
#define CL_H_

#include <llvm/ADT/StringRef.h>

#include <string>
#include <vector>

#include "Util/util.h"

namespace borealis {
namespace driver {

class CommandLine {
public:
    typedef std::vector<const char*> args_t;

private:
    std::vector<const char*> args;
    std::list<std::string> additional_data;

    typedef CommandLine self;

    CommandLine(const args_t& args, const std::list<std::string>& additional_data):
            args{args},
            additional_data{additional_data} {};
    CommandLine(args_t&& args, std::list<std::string>&& additional_data):
            args{std::move(args)},
            additional_data{std::move(additional_data)} {};

public:
    CommandLine() = default;
    CommandLine(int argc, const char** argv):
        args{argv, argv + argc},
        additional_data{} {};

    explicit CommandLine(const char* singleton):
            args{singleton},
            additional_data{} {};
    explicit CommandLine(const std::string& str):
            args{},
            additional_data{} {
        additional_data.push_back(str);
        args.push_back(additional_data.back().c_str());
    }

    CommandLine(const self& that): args(), additional_data(that.args.begin(), that.args.end()) {
        for(auto&& arg: additional_data) args.push_back(arg.c_str());
    }
    CommandLine(self&&) = default;

    CommandLine(const std::vector<const char*>& theArgs):
            args{theArgs},
            additional_data{} {};
    CommandLine(std::vector<const char*>&& theArgs):
            args{std::move(theArgs)},
            additional_data{} {};

    CommandLine(const std::vector<std::string>& theArgs):
        args{},
        additional_data{theArgs.begin(), theArgs.end()} {
        args.reserve(additional_data.size());
        for (const auto& arg: additional_data) args.push_back(arg.c_str());
    };

    CommandLine(const std::list<std::string>& theArgs):
            args{},
            additional_data{theArgs} {
        args.reserve(additional_data.size());
        for (const auto& arg: additional_data) args.push_back(arg.c_str());
    };
    CommandLine(std::list<std::string>&& theArgs):
            args{},
            additional_data{std::move(theArgs)} {
        args.reserve(additional_data.size());
        for (const auto& arg: additional_data) args.push_back(arg.c_str());
    };

    CommandLine& operator=(CommandLine&&) = default;
    CommandLine& operator=(const CommandLine&) = default;

    static CommandLine keepAll(int argc, const char* const* argv) {
        CommandLine ret;
        ret.additional_data.insert(ret.additional_data.begin(), argv, argv+argc);
        ret.args.reserve(argc);
        for (const auto& datum : ret.additional_data) {
            ret.args.push_back(datum.c_str());
        }
        return std::move(ret);
    }

    template<size_t N>
    self suffixes(const char (&prefix)[N]) const {
        return suffixes(prefix, N-1);
    }

    self suffixes(const char* prefix, size_t len) const {
        args_t newArgs;
        for (llvm::StringRef sr : args) {
            if (sr.startswith(prefix)) {
                newArgs.push_back(sr.drop_front(len).data());
            }
        }
        return self{ std::move(newArgs) };
    }

    template<size_t N>
    self unprefix(const char (&prefix)[N]) const {
        return unprefix(prefix);
    }

    self unprefix(const char* prefix) const {
        args_t newArgs;
        for (llvm::StringRef sr : args) {
            if (!sr.startswith(prefix)) {
                newArgs.push_back(sr.data());
            }
        }
        return self{ std::move(newArgs) };
    }

    self push_back(const std::string& data) const {
        self copy = *this;
        copy.additional_data.push_back(data);
        copy.args.push_back(copy.additional_data.back().c_str());
        return std::move(copy);
    }

    self push_back_unsafe(const char* add) const {
        self copy = *this;
        copy.args.push_back(add);
        return std::move(copy);
    }

    self push_front(const std::string& data) const {
        self copy = *this;
        copy.additional_data.push_back(data);
        copy.args.insert(copy.args.begin(), copy.additional_data.back().c_str());
        return std::move(copy);
    }

    self ifempty(const self& data) const {
        if (args.empty()) return data;
        else return *this;
    }

    self tail() const {
        return self{ args_t{ std::next(args.begin()), args.end() }, additional_data };
    }

    self nullTerminated() const {
        return push_back_unsafe(nullptr);
    }

    const char* const* argv() const {
        return args.data();
    }

    int argc() const {
        return args.size();
    }

    bool empty() const {
        return args.empty();
    }

    borealis::util::option<const char*> single() const {
        if (args.empty()) return util::nothing();
        return util::just(args.back());
    }

    const char* single(const char* else_) const {
        if (args.empty()) return else_;
        return args.back();
    }

    std::vector<std::string> stlRep() const {
        std::vector<std::string> ret{};
        ret.reserve(args.size());
        for (const auto& arg : args) ret.push_back(arg);
        return std::move(ret);
    }

    const std::vector<const char*>& data() const {
        return args;
    }

    self operator+(const self& that) const {
        self ret = *this;
        for (const auto& arg : that.args) {
            ret.additional_data.push_back(arg);
            ret.args.push_back(ret.additional_data.back().c_str());
        }
        return std::move(ret);
    }

    // FIXME: templated version broken due to operator ambiguity
    friend std::ostream& operator<<(std::ostream& str, const CommandLine& cl) {
        if (cl.empty()) {
            str << "<empty command line invocation>";
        } else {
            str << util::head(cl.args);
            for (const auto& arg : util::tail(cl.args)) {
                str << " " << arg;
            }
        }
        // this is generally fucked up
        return str;
    }
};

} /* namespace driver */
} /* namespace borealis */

#endif /* CL_H_ */
