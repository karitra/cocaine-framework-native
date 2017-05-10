/*
    Copyright (c) 2015 Evgeny Safronov <division494@gmail.com>
    Copyright (c) 2011-2015 Other contributors as noted in the AUTHORS file.
    This file is part of Cocaine.
    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cocaine/framework/worker/options.hpp"

#include <array>
#include <iostream>

#include <boost/program_options.hpp>

#include "defaults.hpp"

using namespace cocaine::framework;

namespace cocaine { namespace framework { namespace details {
    constexpr auto DEFAULT_TOKENS_SRV_NAME = "tvm";
    constexpr auto DEFAULT_REFRESH_INTERVAL_SEC = 15u;

    constexpr auto DEFAULT_TOKEN_BODY = "very_secret";

    constexpr auto KEY_ENV_TOKEN_TYPE = "COCAINE_APP_TOKEN_TYPE";
    constexpr auto KEY_ENV_TOKEN_BODY = "COCAINE_APP_TOKEN_BODY";
}
}
}

namespace {

void help(const char* program, const boost::program_options::options_description& description) {
    std::cerr << "Usage: " << program
              << " --app APP --uuid UUID --endpoint ENDPOINT --locator LOCATOR"
              << std::endl
              << std::endl;
    std::cerr << description << std::endl;
}

} // namespace

options_t::options_t(int argc, char** argv)
{
    boost::program_options::options_description options("Configuration");
    options.add_options()
        ("app",      boost::program_options::value<std::string>(),   "application name")
        ("uuid",     boost::program_options::value<std::string>(),   "worker uuid")
        ("endpoint", boost::program_options::value<std::string>(),   "cocaine-runtime endpoint")
        ("locator",  boost::program_options::value<std::string>(),   "locator endpoints")
        ("protocol", boost::program_options::value<std::uint32_t>(), "protocol version");

    boost::program_options::options_description general("General options");
    general.add(options);
    general.add_options()
        ("help,h",     "display this help and exit")
        ("version,v",  "output the Framework version information and exit");

    boost::program_options::command_line_parser parser(argc, argv);
    parser.options(general);

    boost::program_options::variables_map vm;
    boost::program_options::store(parser.run(), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        help(argv[0], general);
        std::exit(0);
    }

    if (vm.count("version")) {
        std::cerr << "0.12.0" << std::endl;
        std::exit(0);
    }

    const std::array<const char*, 4> required = {{ "app", "uuid", "endpoint", "locator" }};
    for (uint id = 0; id < required.size(); ++id) {
        auto option = required[id];

        if (vm.count(option) == 0) {
            std::cerr << "ERROR: the required '" << option << "' option is not specified"
                      << std::endl << std::endl;
            help(argv[0], general);
            std::exit(id);
        }
    }

    std::uint32_t protocol = 0;
    if (vm.count("protocol")) {
        protocol = vm["protocol"].as<std::uint32_t>();

        if (protocol > 2) {
            std::cerr << "ERROR: protocol version mismatch" << std::endl << std::endl;
            std::exit(1);
        }
    }

    name     = vm["app"].as<std::string>();
    uuid     = vm["uuid"].as<std::string>();
    endpoint = vm["endpoint"].as<std::string>();
    locator  = vm["locator"].as<std::string>();

    other["protocol"] = protocol;

    const char *env_val = nullptr;

    if ((env_val = std::getenv(details::KEY_ENV_TOKEN_TYPE)) != nullptr) {
        other["token_type"] = std::string(env_val);
    } else {
        other["token_type"] = std::string(details::DEFAULT_TOKEN_TYPE);
    }

    if ((env_val = std::getenv(details::KEY_ENV_TOKEN_BODY)) != nullptr) {
        other["token_body"] = std::string(env_val);
    } else {
        other["token_body"] = std::string(details::DEFAULT_TOKEN_BODY);
    }

    other["tokens_service_name"] = std::string(details::DEFAULT_TOKENS_SRV_NAME);
    other["refresh_ticket_interval_sec"] = unsigned(details::DEFAULT_REFRESH_INTERVAL_SEC);
}

std::uint32_t
options_t::protocol() const {
    return boost::any_cast<std::uint32_t>(other.at("protocol"));
}

const boost::any&
options_t::at(const std::string& name) const {
    return other.at(name);
}
