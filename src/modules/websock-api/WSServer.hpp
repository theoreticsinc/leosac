/*
    Copyright (C) 2014-2016 Leosac

    This file is part of Leosac.

    Leosac is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leosac is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "LeosacFwd.hpp"
#include "Messages.hpp"
#include "Service.hpp"
#include "WebSockFwd.hpp"
#include "api/APIAuth.hpp"
#include "api/APISession.hpp"
#include "api/CRUDResourceHandler.hpp"
#include "api/MethodHandler.hpp"
#include "core/APIStatusCode.hpp"
#include "core/audit/AuditFwd.hpp"
#include "tools/db/db_fwd.hpp"
#include <boost/optional.hpp>
#include <set>
#include <type_traits>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <zmqpp/zmqpp.hpp>

namespace Leosac
{
namespace Module
{
namespace WebSockAPI
{
using json = nlohmann::json;

/**
 * The implementation class that runs the websocket server.
 * The `run()` method is invoked in its own thread, and from this point on, the
 * object lives its life independently in this thread.
 *
 * The WebSockAPIModule object can communicate with WSServer by calling any of the
 * thread safe method. However, for WSServer --> WebSocketAPIModule communicate,
 * the WSServer object uses a PUSH socket.
 *
 * @note Unless specified otherwise, the methods in this class ARE NOT thread-safe.
 */
class WSServer
{
  public:
    /**
     * @param database A (non-null) pointer to the
     * database.
     */
    WSServer(WebSockAPIModule &module, DBPtr database);
    ~WSServer();

    using Server           = websocketpp::server<websocketpp::config::asio>;
    using ConnectionAPIMap = std::map<websocketpp::connection_hdl, APIPtr,
                                      std::owner_less<websocketpp::connection_hdl>>;

    void run(const std::string &interface, uint16_t port);

    Server srv_;

    /**
     * Start the process of shutting down the server.
     *
     * The server will stop listening for new connection and will
     * attempt to close existing one.
     *
     * @note This method can safely be called from an other thread.
     */
    void start_shutdown();

    /**
     * Register an handler on behalf on an other module.
     *
     * This is used as part of the websocket passthrough feature.
     *
     * @note This method is thread-safe.
     *
     * @param name The name of the handler (ie, the message's `type`)
     * @param client_id An opaque identifier that identifies the client (ie module)
     *        on the WebSocketAPI router socket.
     */
    void register_external_handler(const std::string &name,
                                   const std::string &client_id);

    /**
     * This function block the calling thread until the WebSocket thread has
     * processed the handler registration.
     */
    bool ASIO_REGISTER_HANDLER(const Service::WSHandler &handler,
                               const std::string &name);

    /**
     * Send a message to a connection identified by `connection_identifier`.
     *
     * @note This method is thread-safe.
     */
    void send_external_message(const std::string &connection_identifier,
                               const std::string &content);

    /**
     * Retrieve the authentication helper.
     */
    APIAuth &auth();

    /**
     * Retrieve database handle
     */
    DBPtr db();

    /**
     * Retrieve database service pointer.
     */
    DBServicePtr dbsrv();

    /**
     * Retrieve the CoreUtils pointer.
     */
    CoreUtilsPtr core_utils();

    /**
     * Deauthenticate all the connections of `user`, except
     * the `exception` APISession.
     *
     * @param new_transaction If set to true, a new odb::transaction will be used to
     * delete the authentication tokens from the database. If this is false, the
     * currently active transaction is used, and it is the caller responsibility to
     * make
     * sure that the transaction will be commited.
     * Invoking this method with \new_transaction` being false and no currently
     * active
     * transaction is not allowed.
     */
    void clear_user_sessions(Auth::UserPtr user, APIPtr exception,
                             bool new_transaction = true);

    /**
     * Can be used by internal handlers to send a message to a give
     * Leosac module.
     */
    void send_to_module(const std::string &module_name, zmqpp::message msg);

  private:
    void on_open(websocketpp::connection_hdl hdl);

    void on_close(websocketpp::connection_hdl hdl);

    /**
     * A websocket message has been received.
     *
     * At this point, all error handling happens through the use of exception.
     * Non-exceptional event, such as:
     *   + Malformed packet
     *   + Unsuficient permission to perform a given API call
     *   + ...
     * are also reported through exceptions.
     *
     * While this may not be the best performance wise, it's
     * unlikely to be a bottleneck, but it helps keep things clean.
     *
     * @note This method is responsible for saving the WSAPICall Audit event.
     */
    void on_message(websocketpp::connection_hdl hdl, Server::message_ptr msg);

    /**
     * Handle a request.
     *
     * Extract request header, set-up exception handling for api handler
     * invocation.
     *
     * Optionally returns a ServerMessage that must be send to the client.
     */
    boost::optional<ServerMessage> handle_request(APIPtr api_handle,
                                                  const ClientMessage &msg,
                                                  Audit::IAuditEntryPtr);

    /**
     * Create a ClientMessage object from a json request.
     */
    ClientMessage parse_request(const json &in);

    /**
     * Send a message over a connection.
     * @param hdl The connection
     * @param msg The message.
     */
    void send_message(websocketpp::connection_hdl hdl, const ServerMessage &msg);

    /**
     * An internal helper function to register a CRUD resource handler.
     *
     * The resource name will be expanded to create handler for multiple type of
     * requests:
     *     + "resource_name"_read
     *     + "resource_name"_create
     *     + ...
     *
     * @param resource_name
     * @param factory
     */
    void register_crud_handler(const std::string &resource_name,
                               CRUDResourceHandler::Factory factory);

    /**
     * Dispatch the request from a client, so that it is processed by
     * the appropriate handler.
     *
     * This method returns an `optional` json object. If the handler is external,
     * then no json is returned.
     */
    boost::optional<json> dispatch_request(APIPtr api_handle,
                                           const ClientMessage &in,
                                           Audit::IAuditEntryPtr);

    /**
     * Returns true if an handler named `name` already
     * exists.
     */
    bool has_handler(const std::string &name) const;

    /**
     * Find a connection from its assigned identifier.
     */
    websocketpp::connection_hdl
    find_connection(const std::string &connection_identifier) const;

    /**
     * Extract values from the `msg` and finalizes the `audit` object with them.
     *
     * The `msg` may be modified if finalizing the audit object fails.
     */
    void finalize_audit(const Audit::IWSAPICallPtr &audit, ServerMessage &msg);

    ConnectionAPIMap connection_session_;
    APIAuth auth_;

    /**
     * This maps (string) command name to API method.
     */
    std::map<std::string, json (APISession::*)(const json &)> handlers_;

    std::map<std::string, MethodHandler::Factory> individual_handlers_;

    std::map<std::string, CRUDResourceHandler::Factory> crud_handlers_;

    /**
     * Handler registered on behalf of other Leosac's modules.
     *
     * This maps the handler's name to the `client_id` key, where `client_id`
     * is the zeroMQ generated identifier for the module that registered the
     * endpoint.
     */
    std::map<std::string, std::string> external_handlers_;

    std::map<std::string, Service::WSHandler> asio_handlers_;

    /**
     * Database service object.
     */
    DBServicePtr dbsrv_;

    /**
     * A reference to the module.
     *
     * The module is guaranteed to outlive the WSServer.
     */
    WebSockAPIModule &module_;

    /**
     * A PUSH socket that connects to the WebSockAPIModule.
     * Connects to "inproc://MODULE.WEBSOCKET.INTERNAL_PULL".
     */
    std::unique_ptr<zmqpp::socket> to_module_;
};
}
}
}
