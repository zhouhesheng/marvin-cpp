#include "message.hpp"
#include "forward_helpers.hpp"
#include "roundtrip.hpp"

using namespace Marvin;
using namespace Marvin::Http;

std::shared_ptr<Client> do_client_connect(std::string code, boost::asio::io_service& io)
{
    Marvin::Uri uri("http://whiteacorn/utests/echo/test.php");
    std::shared_ptr<Client> client = std::shared_ptr<Client>(new Client(io, uri));
    std::shared_ptr<MessageBase> msg = std::shared_ptr<MessageBase>(new MessageBase());
    helpers::applyUriProxy(msg, uri);
    client->asyncConnect([client, msg, code](Marvin::ErrorType& ec){
#if 1 //VERBOSE
        std::cout << "request " << "Error " << ec.value() << " " << ec.message() << std::endl;
        std::cout << "request " << std::hex << client.get() << std::endl;
//        std::cout << "request " << std::hex << &resp << std::endl;
//        std::cout << "request " << resp.statusCode() << " " << resp.status() << std::endl;
//        std::cout << "request " << resp.getBody() << std::endl;
//        std::cout << "request " << std::hex << req.get() << std::endl;
#endif
    });

//    req->setMethod(HttpMethod::GET);
//    req->setUrl("http://whiteacorn/utests/echo/test.php?code="+code);
//
//    std::string b("");
//    req->setContent(b);
//    req->go([ req, code](Marvin::ErrorType& ec){
    return client;

}

std::shared_ptr<Client> one_roundtrip(std::string code, boost::asio::io_service& io)
{
    Marvin::Uri uri("http://whiteacorn/utests/echo/");
    std::shared_ptr<Client> client = std::shared_ptr<Client>(new Client(io, uri ));
    
    std::shared_ptr<MessageBase> msg = std::shared_ptr<MessageBase>(new MessageBase());
    
    msg->setMethod(HttpMethod::GET);
    helpers::applyUriProxy(msg, uri);
    msg->setHeader(Marvin::Http::Headers::Name::Connection, Marvin::Http::Headers::Value::ConnectionClose);
    msg->setHeader(Marvin::Http::Headers::Name::AcceptEncoding, "identity");
    msg->setHeader(Marvin::Http::Headers::Name::TE, "");
    // Http versions defaults to 1.1, so force it to the same as the request
    msg->setContent("");

    std::function<void(Marvin::ErrorType& er, MessageReaderSPtr rdr)> f = [client, msg, code](Marvin::ErrorType& ec, MessageReaderSPtr rdr) {
#ifdef VERBOSE
        std::cout << "request " << "Error " << ec.value() << " " << ec.message() << std::endl;
        std::cout << "request " << std::hex << client.get() << std::endl;
//        std::cout << "request " << std::hex << &resp << std::endl;
//        std::cout << "request " << resp.statusCode() << " " << resp.status() << std::endl;
//        std::cout << "request " << resp.getBody() << std::endl;
//        std::cout << "request " << std::hex << req.get() << std::endl;
#endif
        MessageReaderSPtr b = client->getResponse();
//        std::string bdy = b->getContent();
        auto st = b->statusCode();
        REQUIRE(b->statusCode() == 200);
        
    };
    client->asyncWrite(msg, f);
    return client;
}
/// \brief makes a GET request to the url provided and expects status=200 and a non empty content
std::shared_ptr<Client> general_roundtrip(boost::asio::io_service& io, std::string request_url)
{
    Marvin::Uri uri(request_url);
    std::shared_ptr<Client> client = std::shared_ptr<Client>(new Client(io, uri ));
    
    std::shared_ptr<MessageBase> msg = std::shared_ptr<MessageBase>(new MessageBase());
    
    msg->setMethod(HttpMethod::GET);
    helpers::applyUriProxy(msg, uri);
    msg->setHeader(Marvin::Http::Headers::Name::Connection, Marvin::Http::Headers::Value::ConnectionClose);
    msg->setHeader(Marvin::Http::Headers::Name::AcceptEncoding, "identity");
    msg->setHeader(Marvin::Http::Headers::Name::TE, "");
    // Http versions defaults to 1.1, so force it to the same as the request
    msg->setContent("");

    std::function<void(Marvin::ErrorType& er, MessageReaderSPtr rdr)> f = [client, msg](Marvin::ErrorType& ec, MessageReaderSPtr rdr) {
        MessageReaderSPtr b = client->getResponse();
        std::string bdy = (b->getContent())->to_string();
        auto st = b->statusCode();
        REQUIRE(b->statusCode() == 200);
        REQUIRE( bdy.size() > 0);
        
    };
    client->asyncWrite(msg, f);
    return client;
}

TEST_CASE("ssl_ssllabs", "[first]")
{
    boost::asio::io_service io_service;
    std::vector<std::shared_ptr<Client>> rt;
    auto c = general_roundtrip(io_service, "https://www.ssllabs.com/");
    io_service.run();
    rt.clear();
}
TEST_CASE("ssl_ssltest", "[first]")
{
    boost::asio::io_service io_service;
    std::vector<std::shared_ptr<Client>> rt;
    auto c = general_roundtrip(io_service, "https://ssltest/");
    io_service.run();
    rt.clear();

}

TEST_CASE("ClientRoundTrip-SixTimes","")
{
    boost::asio::io_service io_service;
    std::vector<std::shared_ptr<Client>> rt;
    rt.push_back(one_roundtrip("1", io_service));
#if 0
    rt.push_back(one_roundtrip("A", io_service));
    rt.push_back(one_roundtrip("B", io_service));
    rt.push_back(one_roundtrip("C", io_service));
    rt.push_back(one_roundtrip("D", io_service));
    rt.push_back(one_roundtrip("E", io_service));
#endif
    io_service.run();
    rt.clear();
}

