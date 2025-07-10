// ======================= exchange.cpp =======================
#include "exchange.hpp"

namespace hz_mq {

// ---------- exchange ----------
exchange::exchange(const std::string& ename, ExchangeType etype, bool edurable,
                   bool eauto_delete,
                   const std::unordered_map<std::string, std::string>& eargs)
    : name(ename),
      type(etype),
      durable(edurable),
      auto_delete(eauto_delete),
      args(eargs) {}

void exchange::set_args(const std::string& str_args)
{
    size_t start = 0;
    while (start < str_args.size()) {
        size_t pos = str_args.find('&', start);
        std::string pair = str_args.substr(start, pos - start);
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq);
            std::string val = pair.substr(eq + 1);
            args[key] = val;
        }
        if (pos == std::string::npos) break;
        start = pos + 1;
    }
}

std::string exchange::get_args() const
{
    std::string result;
    for (auto it = args.begin(); it != args.end(); ++it) {
        result += it->first + "=" + it->second;
        if (std::next(it) != args.end()) result += "&";
    }
    return result;
}

// ---------- exchange_mapper ----------
exchange_mapper::exchange_mapper(const std::string& dbfile)
{
    (void)dbfile;  // 真实实现中可在此打开 DB、建表等
}

bool exchange_mapper::insert(exchange::ptr& ex)
{
    (void)ex;
    return true;   // 示例总是成功
}

void exchange_mapper::remove(const std::string& name)
{
    (void)name;
}

exchange_map exchange_mapper::all()
{
    return {};     // 示例：无持久化数据
}

// ---------- exchange_manager ----------
exchange_manager::exchange_manager(const std::string& dbfile)
    : __mapper(dbfile)
{
    __exchanges = __mapper.all();
}

bool exchange_manager::declare_exchange(const std::string& name, ExchangeType type,
                                        bool durable, bool auto_delete,
                                        const std::unordered_map<std::string, std::string>& args)
{
    std::unique_lock<std::mutex> lock(__mtx);

#if __cpp_lib_unordered_map_contains >= 201411L  // C++20
    if (__exchanges.contains(name)) return true;
#else
    if (__exchanges.find(name) != __exchanges.end()) return true;
#endif

    auto exptr = std::make_shared<exchange>(name, type, durable, auto_delete, args);

    if (durable && !__mapper.insert(exptr)) return false;

    __exchanges[name] = std::move(exptr);
    return true;
}

void exchange_manager::delete_exchange(const std::string& name)
{
    std::unique_lock<std::mutex> lock(__mtx);

    auto it = __exchanges.find(name);
    if (it == __exchanges.end()) return;

    if (it->second->durable) __mapper.remove(name);
    __exchanges.erase(it);
}

exchange::ptr exchange_manager::select_exchange(const std::string& name)
{
    std::unique_lock<std::mutex> lock(__mtx);

    auto it = __exchanges.find(name);
    return (it == __exchanges.end()) ? nullptr : it->second;
}

exchange_map exchange_manager::all()
{
    std::unique_lock<std::mutex> lock(__mtx);
    return __exchanges;
}

bool exchange_manager::exists(const std::string& name)
{
    std::unique_lock<std::mutex> lock(__mtx);
#if __cpp_lib_unordered_map_contains >= 201411L
    return __exchanges.contains(name);
#else
    return __exchanges.find(name) != __exchanges.end();
#endif
}

} 
