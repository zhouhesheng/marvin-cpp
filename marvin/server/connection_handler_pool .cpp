//
//  connection_pool.cpp
//  MarvinCpp
//
//  Created by ROBERT BLACKWELL on 12/14/16.
//  Copyright © 2016 Blackwellapps. All rights reserved.
//
#include <map>
#include <set>
#include "connection_handler_pool.hpp"

#include "rb_logger.hpp"

RBLOGGER_SETLEVEL(LOG_LEVEL_INFO)

#include "i_socket.hpp"


//---------------------------------------------------------------------------------------------------
// InUseConnections - List of assigned connections currently "in use"
//---------------------------------------------------------------------------------------------------
InUseConnectionsType::InUseConnectionsType()
{
}
std::size_t
InUseConnectionsType::size()
{
    return _connections.size();
}

void
InUseConnectionsType::remove(ISocket* aConn)
{
    if( aConn == NULL )
        return;
    if( _connections.find(aConn) == _connections.cend() ) { assert(false);}
    _connections.erase(aConn);
    
}

void
InUseConnectionsType::add(ISocket* conn)
{
    _connections[conn] = conn;
}

//---------------------------------------------------------------------------------------------------
// ConnectionRequest - Holds a pending request for a connection
//---------------------------------------------------------------------------------------------------
ConnectionHandlerRequest::ConnectionRequest(
    std::string scheme,
    std::string server,
    std::string service,
    ConnectCallbackType cb)
{
    _scheme = scheme;
    _server = server;
    _service = service;
    _callback = cb;
}
ConnectionHandlerRequest::~ConnectionRequest()
{
    LogDebug("");
}
//---------------------------------------------------------------------------------------------------
// WaitingRequests - List of requests for a connection that have been put into "wait"
//---------------------------------------------------------------------------------------------------
WaitingRequestsType::WaitingRequestsType()
{
}
    
std::size_t
WaitingRequestsType::size()
{
    return _waitingRequests.size();
}
ConnectionRequest*
WaitingRequestsType::find(std::string scheme, std::string server, std::string service)
{
    LogDebug(" size:", _waitingRequests.size());
    for( auto itr = _waitingRequests.begin(); itr < _waitingRequests.end(); itr++ ){
        auto ent = *itr;
        if( (ent->_scheme == scheme) && (ent->_server == server) && (ent->_service == service) ){
            _waitingRequests.erase(itr);
            return ent;
        }
    }
    return nullptr;
}

ConnectionRequest*
WaitingRequestsType::removeOldest()
{
    LogDebug(" size:", _waitingRequests.size());
    auto r =  _waitingRequests[0];
    _waitingRequests.erase(_waitingRequests.begin());
    return r;
}
    
void
WaitingRequestsType::add(ConnectionRequest* connReq)
{
    LogDebug(" size:", _waitingRequests.size());
    _waitingRequests.push_back(connReq);
}


#ifdef NOTYET
//---------------------------------------------------------------------------------------------------
// map of counters by assigned host - that is host that have been assigned to a connection
// where the connection is either inuse of idle
//---------------------------------------------------------------------------------------------------
class HostsCounterType
{
    private:
    std::map<std::string, std::size_t>  _count;
    
    public:
    HostsCounterType(){}
    void addHost(std::string hostId)
    {
        if( _count.find(hostId) != std::map::end { assert(false);}
        _count[hostId] = 0;
    }
    void removeHost(std::string hostId)
    {
        if( _count.erase(hostId) != 1) { assert(false); };
    }
    void incrementHost(std::string hostId)
    {
        if( _count.find(hostId) != std::map::end { assert(false);}
        _count[hostId] = _count[hostId]++;
    }
    void decrementHost(std::string hostId)
    {
        if( _count.find(hostId) != std::map::end { assert(false);}
        _count[hostId]-- = _count[hostId]--;
    }
    std::size_t countFor(std::string hostId)
    {
        if( _count.find(hostId) != std::map::end { assert(false);}
        return _count[hostId];
    }
    
};
#endif
ConnectionHandlerManager* globalConnectionPool = NULL;

ConnectionHandlerManager::ConnectionHandlerManager(boost::asio::io_service& io_service): io(io_service), resolver_(io_service), _poolStrand(io)
{
    _maxConnections = 25;
}
ConnectionPool* ConnectionHandlerManager::::getInstance(boost::asio::io_service& io)
{
    if( globalConnectionPool == NULL ){
        globalConnectionPool = new ConnectionPool(io);
    }
    return globalConnectionPool;
}
/**
 * get a connection to the scheme::server
 */
void ConnectionHandlerManager::::asyncGetConnection(
            std::string scheme, // http: or https:
            std::string server, // also called hostname
            std::string service,// http/https or port number
            ConnectCallbackType cb
)
{
    auto hf = _poolStrand.wrap(std::bind(&ConnectionPool::__asyncGetConnection, this, scheme, server, service, cb));
    io.post(hf);
}
void ConnectionHandlerManager::::__asyncGetConnection(
            std::string scheme, // http: or https:
            std::string server, // also called hostname
            std::string service,// http/https or port number
            ConnectCallbackType cb
)
{
    LogDebug(" inUser size : ", _inUse.size());
    if( _inUse.size() >= _maxConnections){
        auto r = new ConnectionRequest(scheme, server, service, cb);
        _waitingRequests.add(r);
    }else{
        createNewConnection(scheme, server, service, cb);
    }
}
void ConnectionHandlerManager::::createNewConnection(
            std::string scheme, // http: or https:
            std::string server, // also called hostname
            std::string service,// http/https or port number
            ConnectCallbackType cb
){
            //    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(),
            //                                                server,
            //                                                service,
            //                                                tcp::resolver::query::canonical_name);
    
    ISocket* conn = socketFactory(io, scheme, server, service);
    //
    // a bunch of logic here about find existing, add to connection table etc
    //
    _inUse.add(conn);
    //
    //
    conn->asyncConnect([this, conn, cb](Marvin::ErrorType& ec, ISocket* conn){
        if( !ec ){
            postSuccess(cb, conn);
        }else{
            postFail(cb, ec);
        }
    });
}

//
//so far all we implement is a limit on the number of connections
//
void ConnectionHandlerManager::::releaseConnection(ISocket* conn)
{
    auto hf = _poolStrand.wrap(std::bind(&ConnectionPool::__releaseConnection, this, conn));
    io.post(hf);

}
void ConnectionHandlerManager::::__releaseConnection(ISocket* conn)
{
    LogDebug(" conn: ", conn);
    assert( conn != NULL );
    _inUse.remove(conn);
//    delete conn;
    if( _waitingRequests.size() > 0 )
    {
        LogDebug(" dispatch a waiting connection request size: ", _waitingRequests.size());
        //
        // get the req that has been waiting the lngests
        //
        auto req = _waitingRequests.removeOldest();
        std::string scheme = req->_scheme;
        std::string server = req->_server;
        std::string service = req->_service;
        ConnectCallbackType cb = req->_callback;
        delete req;
        createNewConnection(scheme, server, service, cb);
    }
// have not implemented real pooling - where we keep connections open
#ifdef CONN_POOLING
    return;
    if( _waitingRequests.size() > 0 ){
        auto req = _waitingRequests.find(conn->scheme(), conn->server(), conn->service());
        // found a request for the same host
        if( req != nullptr ){
            postSuccess(req->_callback, conn);
            return;
        }else{
            // none of the request are for same host
            // destroy releasing conn and create a new one
            
            // could try and be trickey here and repurpose the conn
            
            _inUse.remove(conn);
            delete conn;
            // get oldest waiting request
            req = _waitingRequests.removeOldest();
            // create a connection for it
            createNewConnection(req->_scheme, req->_server, req->_service, req->_callback);
        }
    }else{
        LogDebug("");
        _inUse.remove(conn);
        delete conn;
    }
#endif
}
void ConnectionHandlerManager::::postSuccess(ConnectCallbackType cb, ISocket* conn)
{
    Marvin::ErrorType merr = Marvin::make_error_ok();
    auto pf = std::bind(cb, merr, conn);
    io.post(pf);
}
void ConnectionHandlerManager::::postFail(ConnectCallbackType cb, Marvin::ErrorType& ec)
{
    Marvin::ErrorType merr = ec;
    auto pf = std::bind(cb, merr, nullptr);
    io.post(pf);
}

