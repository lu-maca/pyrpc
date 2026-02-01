#pragma once
#include <Arduino.h>
#include <MsgPack.h>

namespace pyrpc {
namespace internals {

struct RpcEntry {
  void *func;
  void (*handler)(void *);
  const __FlashStringHelper *description;
};

static inline MsgPack::Packer packer;
static inline MsgPack::Unpacker unpacker;
// map_t is defined in MsgPack/Types.h and already takes care about std::map existance
// Same is true for MsgPack::str_t
static inline MsgPack::map_t<MsgPack::str_t, RpcEntry> rpc_table{};

template <typename... Args>
std::tuple<Args...> deserialize_as_tuple(MsgPack::Unpacker &u) {
  std::tuple<Args...> t;

  bool ok =
      std::apply([&](Args &...elems) { return u.deserialize(elems...); }, t);

  if (!ok) {
    // error TBD
  }
  return t;
}

void reply(MsgPack::bin_t<uint8_t> packet) {
  const uint16_t size = packet.size();
  Serial.write(static_cast<uint8_t>(size >> 8));
  Serial.write(static_cast<uint8_t>(size & 0xFF));
  Serial.write(packet.data(), packet.size());
}

template <typename R, typename... Args> struct RpcWrapper {
  static void call(void *f) {
    auto func = reinterpret_cast<R (*)(Args...)>(f);

    if constexpr (sizeof...(Args) == 0) {
      // no arguments
      if constexpr (arx::stdx::is_void<R>::value) {
        func();
      } else {
        R r = func();
        packer.clear();
        packer.serialize(r);
        const auto serialized = packer.packet();
        reply(serialized);
      }
    } else {
      // arguments, unpack them and call the function
      auto args = deserialize_as_tuple<Args...>(unpacker);

      if constexpr (arx::stdx::is_void<R>::value) {
        std::apply(func, args);
      } else {
        R r = std::apply(func, args);
        // manage result here
        packer.clear();
        packer.serialize(r);
        const auto serialized = packer.packet();
        reply(serialized);
      }
    }
    Serial.flush();
  }
};

void dispatch(const MsgPack::str_t name) {
  auto it = internals::rpc_table.find(name);
  if (it == internals::rpc_table.end()) {
    // TBD error
    return;
  }

  it->second.handler(it->second.func);
}

} // namespace internals

template <typename R, typename... Args>
void register_rpc(const MsgPack::str_t name, R (*func)(Args...),
                  const __FlashStringHelper *description) {
  const internals::RpcEntry entry {
    .func = reinterpret_cast<void *>(func),
    .handler = &pyrpc::internals::RpcWrapper<R, Args...>::call,
    .description = description
  };

  internals::rpc_table.emplace(name, entry);
}

inline void begin() {
  struct Help {
    static MsgPack::str_t call() {
      MsgPack::str_t descr;
      for (const auto &[key, val] : internals::rpc_table) {
        descr += "@entry " + key + " - " + val.description + "\n";
      }
      return descr;
    }
  };
  register_rpc("help", &Help::call,
               F("@brief Built-in method describing all available procedures"));
}

inline void process() {
  struct RpcRequest {
    MsgPack::str_t call;
  };

  static uint16_t len = 0;
  static uint8_t header_bytes = 0;
  static MsgPack::bin_t<uint8_t> rcv;

  if (!Serial.available()) {
    return;
  }

  // read header
  while (Serial.available() && header_bytes < 2) {
    uint8_t b = Serial.read();
    len = (len << 8) | b;
    header_bytes++;
  }

  if (header_bytes < 2) {
    // incomplete header 
    return; 
  }

  // read payload
  while (Serial.available() && rcv.size() < len) {
    rcv.push_back(Serial.read());
  }

  if (rcv.size() < len) {
    // incomplete payload
    return; 
  }

  internals::unpacker.clear();
  internals::unpacker.feed(rcv.data(), rcv.size());

  RpcRequest request;
  internals::unpacker.deserialize(request.call);
  internals::dispatch(request.call);

  // reset current state for next message
  len = 0;
  header_bytes = 0;
  rcv.clear();
}

} // namespace pyrpc
