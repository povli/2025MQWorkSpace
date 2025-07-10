// ======================= exchange.hpp =======================
#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>

#include "../common/protocol.pb.h"   // ExchangeType 枚举（由 Protobuf 生成）

namespace hz_mq {

// ---------- 交换机元数据 ----------
struct exchange {
    using ptr = std::shared_ptr<exchange>;

    std::string name;
    ExchangeType type{};
    bool durable{false};
    bool auto_delete{false};
    std::unordered_map<std::string, std::string> args;

    exchange() = default;
    exchange(const std::string& ename, ExchangeType etype, bool edurable,
             bool eauto_delete,
             const std::unordered_map<std::string, std::string>& eargs);

    void set_args(const std::string& str_args);
    [[nodiscard]] std::string get_args() const;
};

// 交换机名 → 元数据
using exchange_map = std::unordered_map<std::string, exchange::ptr>;

// ---------- 持久化映射层（示例省略真实存储细节） ----------
class exchange_mapper {
public:
    explicit exchange_mapper(const std::string& dbfile);
    bool insert(exchange::ptr& ex);
    void remove(const std::string& name);
    exchange_map all();
};

// ---------- 内存交换机管理器 ----------
class exchange_manager {
public:
    using ptr = std::shared_ptr<exchange_manager>;

    explicit exchange_manager(const std::string& dbfile);

    bool declare_exchange(const std::string& name, ExchangeType type, bool durable,
                          bool auto_delete,
                          const std::unordered_map<std::string, std::string>& args);

    void delete_exchange(const std::string& name);
    exchange::ptr select_exchange(const std::string& name);
    exchange_map all();
    bool exists(const std::string& name);

private:
    std::mutex __mtx;
    exchange_mapper __mapper;
    exchange_map __exchanges;
};

} 

