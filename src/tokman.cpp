#include <memory>
#include <utility>

#include "cocaine/framework/detail/loop.hpp"
#include "cocaine/framework/detail/log.hpp"

#include "cocaine/framework/service.hpp"
#include "cocaine/framework/manager.hpp"
#include "cocaine/framework/worker.hpp"

#include "cocaine/framework/util/future.hpp"

#include <asio/deadline_timer.hpp>

#include "worker/defaults.hpp"

#include "idl/tvm.hpp"
#include "tokman.hpp"

namespace cocaine { namespace framework {

namespace details {
    auto
    as_string(const options_t& options, const std::string& name) -> std::string
    {
        return boost::any_cast<std::string>(options.at(name));
    }

    auto
    as_uint(const options_t& options, const std::string& name) -> unsigned
    {
        try {
            return boost::any_cast<unsigned>(options.at(name));
        } catch(const boost::bad_any_cast& error) {
            return boost::any_cast<int>(options.at(name));
        }
    }
}

auto
make_token(const options_t& options) -> token_t {
    return token_t{
        details::as_string(options, "token_type"),
        details::as_string(options, "token_body") };
}

struct tvm_token_manager_t::tvm_service_impl_t :
    public std::enable_shared_from_this<tvm_service_impl_t>
{
    std::string application_name;

    service<cocaine::io::tvm_tag> tokens_service;

    asio::deadline_timer refresh_timer;
    boost::posix_time::seconds refresh_interval;

    token_t tok;
    std::mutex tok_mut;

    cocaine::framework::future<void> future;

    tvm_service_impl_t(detail::loop_t& loop, service_manager_t& manager, const options_t options) :
        application_name(options.name),
        tokens_service(manager.create<cocaine::io::tvm_tag>(details::as_string(options, "tokens_service_name"))),
        refresh_timer(loop),
        refresh_interval(details::as_uint(options, "refresh_ticket_interval_sec")),
        tok(make_token(options))
    {}

    auto
    token() -> token_t {
        std::lock_guard<std::mutex> lock(tok_mut);
        return tok;
    }

    auto
    refresh_ticket_async() -> void {
        auto self = shared_from_this();

        refresh_timer.expires_from_now(refresh_interval);
        refresh_timer.async_wait([self] (const std::error_code& ec) {
            CF_DBG("refresh_ticket -> async callback");

            if (ec) {
                CF_DBG("refresh_ticket -> canceled");
                // canceled
                return;
            }

            // Note that as refresh_timer is bound to worker_t io loop,
            // all exception will be (hopefully) propagated to worker_t::run
            // try/catch block.
            if (self->future.valid()) {
                self->future.get();
            }

            self->future = self->tokens_service.invoke<io::tvm::refresh_ticket>(
                self->application_name,
                self->tok.body
            ).then([self] (task<std::string>::future_move_type future) {
                CF_DBG("refresh_ticket -> updating token");

                std::lock_guard<std::mutex> lock(self->tok_mut);
                self->tok.body = future.get();
            });

            self->refresh_ticket_async();
        });
    }
};

auto
token_manager_t::make(detail::loop_t& io, service_manager_t& manager, const options_t& options)
    -> std::shared_ptr<token_manager_t>
{
    if (make_token(options).type == std::string(details::TVM_TOKEN_TYPE)) {
        return std::make_shared<tvm_token_manager_t>(io, manager, options);
    } else {
        return std::make_shared<null_token_manager_t>();
    }
}

auto
null_token_manager_t::token() const -> token_t {
    return token_t();
}

tvm_token_manager_t::tvm_token_manager_t(detail::loop_t& io, service_manager_t& manager, const options_t& options):
    d(std::make_shared<tvm_service_impl_t>(io, manager, options))
{
    d->refresh_ticket_async();
}

auto
tvm_token_manager_t::token() const -> token_t {
    return d->token();
}

}
}
