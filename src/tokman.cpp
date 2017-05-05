#include <memory>
#include <utility>

#include "cocaine/framework/detail/loop.hpp"
#include "cocaine/framework/detail/log.hpp"

#include "cocaine/framework/service.hpp"
#include "cocaine/framework/manager.hpp"
#include "cocaine/framework/worker.hpp"

#include <asio/deadline_timer.hpp>

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
        return boost::any_cast<unsigned>(options.at(name));
    }
}

auto
make_token(const options_t& options) -> token_t {
    return token_t{
        details::as_string(options, "token_type"),
        details::as_string(options, "token_body") };
}

auto
make_null_token(const options_t& options) -> token_t {
    return token_t{
        details::as_string(options, "test_token_type"),
        details::as_string(options, "test_token_body") };
}

struct tvm_token_manager_t::tvm_service_impl_t :
    public std::enable_shared_from_this<tvm_service_impl_t>
{
    using tvm_service_tag = cocaine::io::tvm_tag;
    using tvm_service_type = service<tvm_service_tag>;

    std::string application_name;
    tvm_service_type tokens_service;

    asio::deadline_timer refresh_timer;
    boost::posix_time::seconds refresh_interval;

    // In current implementation it seems there is no need for locking, update
    // and read of token are done within one-threaded worker io loop
    // (see worker_t::run), but to stay compatible with possible worker
    // redesign lock is introduced, it wouldn't induce perfomance hit because of
    // low (seconds based) poll rates.
    token_t tok;
    std::mutex tok_mut;

    tvm_service_impl_t(detail::loop_t& loop, service_manager_t& manager, const options_t options) :
        application_name(options.name),
        tokens_service(manager.create<tvm_service_tag>(details::as_string(options, "tokens_service_name"))),
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
            if (ec) {
                CF_DBG("refresh_ticket -> canceled");
                // canceled
                return;
            }

            // Note that as refresh_timer is bound to worker_t io loop,
            // all exception will be propagated to worker_t::run try/catch block
            self->tokens_service.invoke<io::tvm::refresh_ticket>(
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
    if (make_token(options).type == std::string(details::DEFAULT_TOKEN_TYPE)) {
        return std::make_shared<tvm_token_manager_t>(io, manager, options);
    } else {
        return std::make_shared<null_token_manager_t>(options);
    }
}

null_token_manager_t::null_token_manager_t(options_t options) :
    tok(make_null_token(options))
{}

auto
null_token_manager_t::token() const -> token_t {
    return token_t();
}

tvm_token_manager_t::tvm_token_manager_t(detail::loop_t& io, service_manager_t& manager, options_t options):
    d(std::make_shared<tvm_service_impl_t>(io, manager, std::move(options)))
{
    d->refresh_ticket_async();
}

auto
tvm_token_manager_t::token() const -> token_t {
    return d->token();
}

}
}
