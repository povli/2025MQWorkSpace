// ======================= message.hpp =======================
#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "../common/msg.pb.h"   // Protobuf: Message, BasicProperties, ...

namespace hare_mq {

// 智能指针别名，指向队列中的 Protobuf Message
using message_ptr = std::shared_ptr<Message>;

} 
