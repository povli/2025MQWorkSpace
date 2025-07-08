// ======================= connection.cpp =======================
#include "connection.hpp"
#include "../common/logger.hpp"


namespace hare_mq {

// ---------------------------------------------------------------------------
// connection
// ---------------------------------------------------------------------------
connection::connection(const virtual_host::ptr& host,
                       const consumer_manager::ptr& cmp,
                       const std::shared_ptr<ProtobufCodec>& codec,
                       const muduo::net::TcpConnectionPtr& conn,
                       const thread_pool::ptr& pool)
    : __conn(conn), __codec(codec), __cmp(cmp), __host(host), __pool(pool),
      __channels(std::make_shared<channel_manager>()) {}

connection::~connection() = default;

void connection::basic_response(bool ok, const std::string& rid, const std::string& cid)
{
    basicCommonResponse resp;
    resp.set_rid(rid);
    resp.set_cid(cid);
    resp.set_ok(ok);
    __codec->send(__conn, resp);
}

void connection::open_channel(const openChannelRequestPtr& req)
{
    bool ok = __channels->open_channel(req->cid(), __host, __cmp, __codec, __conn, __pool);
    basic_response(ok, req->rid(), req->cid());
}

void connection::close_channel(const closeChannelRequestPtr& req)
{
    __channels->close_channel(req->cid());
    basic_response(true, req->rid(), req->cid());
}

channel::ptr connection::select_channel(const std::string& cid)
{
    return __channels->select_channel(cid);
}

// ---------------------------------------------------------------------------
// connection_manager
// ---------------------------------------------------------------------------
void connection_manager::new_connection(const virtual_host::ptr& host,
                                        const consumer_manager::ptr& cmp,
                                        const std::shared_ptr<ProtobufCodec>& codec,
                                        const muduo::net::TcpConnectionPtr& conn,
                                        const thread_pool::ptr& pool)
{
    std::unique_lock<std::mutex> lock(__mtx);
    if (__conns.find(conn) != __conns.end()) return;

    __conns[conn] = std::make_shared<connection>(host, cmp, codec, conn, pool);
}

void connection_manager::delete_connection(const muduo::net::TcpConnectionPtr& conn)
{
    std::unique_lock<std::mutex> lock(__mtx);
    __conns.erase(conn);
}

connection::ptr connection_manager::select_connection(const muduo::net::TcpConnectionPtr& conn)
{
    std::unique_lock<std::mutex> lock(__mtx);
    auto it = __conns.find(conn);
    return (it == __conns.end()) ? nullptr : it->second;
}

} 
