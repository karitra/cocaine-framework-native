#ifndef COCAINE_FRAMEWORK_SERVICES_LOGGING_HPP
#define COCAINE_FRAMEWORK_SERVICES_LOGGING_HPP

#include <cocaine/framework/service.hpp>
#include <cocaine/framework/logging.hpp>

#include <cocaine/traits/enum.hpp>

namespace cocaine { namespace framework {

struct logging_service_t :
    public service_t,
    public logger_t
{
    static const unsigned int version = cocaine::io::protocol<cocaine::io::logging_tag>::version::value;

    logging_service_t(std::shared_ptr<service_connection_t> connection,
                      const std::string& source) :
        service_t(connection),
        m_source(source)
    {
        try {
            m_priority = call<cocaine::io::logging::verbosity>().next();
        } catch (...) {
            m_priority = cocaine::logging::debug;
        }
    }

    void
    emit(cocaine::logging::priorities priority,
         const std::string& message)
    {
        if (status() == service_status::disconnected) {
            try {
                m_priority = call<cocaine::io::logging::verbosity>().next();
            } catch (...) {
                m_priority = cocaine::logging::debug;
            }
        }

        call<cocaine::io::logging::emit>(priority, m_source, message);
    }

    cocaine::logging::priorities
    verbosity() const {
        return m_priority;
    }

private:
    std::string m_source;
    cocaine::logging::priorities m_priority;
};

}} // cocaine::framework

#endif // COCAINE_FRAMEWORK_SERVICES_LOGGING_HPP
