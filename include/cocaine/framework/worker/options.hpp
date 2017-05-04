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

#pragma once

#include <string>
#include <unordered_map>

#include <boost/any.hpp>

namespace cocaine {

namespace framework {

namespace details {
    constexpr auto DEFAULT_TOKEN_TYPE = "TVM";
}

struct options_t {
    // taken from environment variables
    std::string token_type;
    std::string token_body;

    // set to defaults currently
    std::string tokens_service_name;
    std::uint64_t refresh_ticket_interval_sec;

    std::string name;
    std::string uuid;
    std::string endpoint;
    std::string locator;

    /// Parses command-line arguments to extract all required settings to be able to start the
    /// worker.
    ///
    /// Can internally terminate the program on invalid command-line arguments, providing an help
    /// message and returning a proper exit code.
    options_t(int argc, char** argv);

    std::uint32_t
    protocol() const;

private:
    std::unordered_map<std::string, boost::any> other;
};

} // namespace framework

} // namespace cocaine
