//
// Start both a server and a number of client tthreads and have each of the client
// threads make a number of requests from the server.
//
#include <thread>
#include <regex>
#include <boost/process.hpp>
#include <marvin/forwarding/forwarding_handler.hpp>
#include <marvin/server/http_server.hpp>
#include <marvin/collector/pipe_collector.hpp>
#include <marvin/collector/collector_base.hpp>
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <marvin/external_src/rb_logger/rb_logger.hpp>
RBLOGGER_SETLEVEL(LOG_LEVEL_WARN)

/**
 * This is the simplest possible test of the http mitm proxy. To run it needs manual intervention.
 * After starting execution of tp_proxy_simple executable the program will hang waiting for
 * http traffic to be proxied through localhost:9992
 *
 * Such traffic can be generated by running the script curl.sh that resides in the
 * tests/test_proxy_alone directory.
 *
 * If you have cloned this project from github you will need to modify the curl.sh script
 * to access a different url as the one that comes in the script is local to my development
 * system.
 *
 * Each time you run curl.sh the traffic between the curl.sh script and the url/server
 * will be displayed on stdout.
 *
 * Inspect that traffic to satisfy yourself that the traffic is being correctly captured.
 *
 * This test can also be used to demonstrate https tunneling. Once the etst is running execute
 * the shell script curl_tunnel.sh
 *
 * This will do a get request against www.bankofamerica.com. You will see the response to this request
 * on the shell scripts stdout  - demonstrating that the proxy connected, passed the request and received and passed on the
 * response.
 * You will aslo see on the proxies stdout "Connect Request" which is all that is passed to the collector when a tunnel
 * connection is made. 
 */
int main( int argc, char* argv[] )
{
    LogTrace("hello");
    VLogDebug("hello");
    RBLogger::enableForLevel(LOG_LEVEL_WARN);

    std::vector<std::regex> re{std::regex("^ssllabs(.)*$")};
    std::vector<int> ports{443, 9443};
    ForwardingHandler::configSet_HttpsPorts(ports);
    ForwardingHandler::configSet_HttpsHosts(re);

    HTTPServer* server_ptr;

    auto proxy_func = [&server_ptr](void* param) {
        server_ptr = new HTTPServer([](boost::asio::io_service& io) {
            CollectorBase* pc = new CollectorBase(io, std::cout);
            auto f = new ForwardingHandler(io, pc);
            return f;
        });
        server_ptr->listen(9992);
    };
    std::thread proxy_thread(proxy_func, nullptr);

    proxy_thread.join();
}

