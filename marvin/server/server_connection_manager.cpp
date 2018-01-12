#include "rb_logger.hpp"
RBLOGGER_SETLEVEL(LOG_LEVEL_WARN)
#include "server_connection_manager.hpp"
/// \todo - the code in this file needs tidying up - too many commented out lines
ServerConnectionManager* ServerConnectionManager::instance;

ServerConnectionManager* ServerConnectionManager::get_instance()
{
    return instance;
}
bool ServerConnectionManager::verify()
{
    return true;
}

ServerConnectionManager::ServerConnectionManager(boost::asio::io_service& io, boost::asio::strand& serverStrand, int max_connections)
    : m_io(io), m_serverStrand(serverStrand),
    m_maxNumberOfConnections(max_connections),
    m_currentNumberConnections(0)
{
    LogTorTrace();
    instance = this;
}

void ServerConnectionManager::allowAnotherConnection(ServerConnectionManager::AllowAnotherCallback cb)
{
    LogDebug(" num conn: ", m_connections.size(), " max: ", m_maxNumberOfConnections);
    assert(m_callback == nullptr);
    if( m_connections.size() > m_maxNumberOfConnections ) {
        LogWarn("max connections exceeded - waiting cb: ", (void*)(&cb));
        m_callback = cb;
    } else {
        cb();
    }
}


/**
 * This method is ALWAYS called from the server strand so do not have to post
 * it on that strand.
 */
void ServerConnectionManager::registerConnectionHandler(ConnectionHandler* connHandler)
{
    LogDebug("");
//    std::cout << "register: fd_list.size() " << _fd_list.size() << " "  << std::endl;
//    std::cout << "register: _connections.size() " << _connections.size() << " "  << std::endl;
//    std::cout << "_register: fd_list.size() " << _fd_list.size() << " "  << std::endl;
//    std::cout << "_register: _connections.size() " << _connections.size() << " "  << std::endl;

    long fd = connHandler->nativeSocketFD();
    assert(m_fd_list.find(fd) == m_fd_list.end());
    assert(m_connections.find(connHandler) == m_connections.end()); // assert not already there
    assert(verify());
    m_connections[connHandler] = std::unique_ptr<ConnectionHandler>(connHandler);
    m_fd_list[fd] = fd;
    assert(verify());
}

/**
 * This is called from a ConnectionHandler running on an arbitary io_service.
 * need to post the real action on the server strand.
 */
void ServerConnectionManager::deregister(ConnectionHandler* ch)
{
    LogDebug("nativeSocket:: ", ch->nativeSocketFD());
    LogDebug("num connections:: ", m_connections.size());
//    std::cout << "deregister: fd_list.size() " << _fd_list.size() << " "  << std::endl;
//    std::cout << "deregister: _connections.size() " << _connections.size() << " "  << std::endl;
//    std::cout << "deregister: fd " << ch->nativeSocketFD() << " " << std::hex << ch << std::endl;
    auto pf = m_serverStrand.wrap(std::bind(&ServerConnectionManager::p_deregister, this, ch));
    m_io.post(pf);
}
#pragma mark - private methods
/**
* Paranoid
*/
void ServerConnectionManager::p_deregister(ConnectionHandler* ch)
{
    LogDebug("");
//    std::cout << "_deregister: fd " << ch->nativeSocketFD() << " " << std::hex << ch << std::endl;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
    long fd = ch->nativeSocketFD();
    auto fd1 = m_fd_list.find(fd);
    auto fd2 = m_fd_list.end();
#pragma clang diagnostic pop
//    std::cout << "_deregister: fd_list.size() " << _fd_list.size() << " "  << std::endl;
//    std::cout << "_deregister: _connections.size() " << _connections.size() << " "  << std::endl;
//    for(auto const & f : _fd_list) {
//        std::cout << f.first << " " << f.second << std::endl;
//    }
//    std::cout << "XXX fds : " << fd1->first << " " << fd2->first << std::endl;
//    LogDebug("assert fd ", fd1, " ", fd2 );
    assert(m_fd_list.find(fd) != m_fd_list.end());
    if(m_fd_list.find(fd) == m_fd_list.end()) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
        auto fd1 = m_fd_list.find(fd);
        auto fd2 = m_fd_list.end();
#pragma clang diagnostic pop
        assert(m_fd_list.find(fd) != m_fd_list.end());
    }
    assert(m_connections.find(ch) != m_connections.end()); // assert is there
    assert(verify());
    
    m_connections.erase(ch);
    m_fd_list.erase(fd);
    LogTrace(" num connections : ", m_connections.size() , " m_callback != null", (m_callback != nullptr));
    if (m_callback && (m_connections.size() < m_maxNumberOfConnections)) {
        auto tmp = m_callback;
        LogTrace("releasing cb: ", (void*)(&tmp) );
//        std::cout << "ServerConnectionManager::releasing callback" << std::endl;
        m_callback = nullptr;
        m_io.post(tmp);
    }
}

void ServerConnectionManager::stop_all()
{
//    for (auto c: _connections)
//        c->stop();
//    _connections.clear();
}

