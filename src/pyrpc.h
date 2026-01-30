#pragma once
#include <Arduino.h>
#include <MsgPack.h>
#include <map>

namespace pyrpc {
namespace internals {

struct RpcEntry {
  void *func;
  void (*handler)(void *);
  const __FlashStringHelper *description;
};

static inline MsgPack::Packer packer;
static inline MsgPack::Unpacker unpacker;
static inline std::map<const String, RpcEntry> rpc_table{};

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

template <typename R, typename... Args> struct RpcWrapper {
  static void call(void *f) {
    auto func = reinterpret_cast<R (*)(Args...)>(f);

    if constexpr (sizeof...(Args) == 0) {
      // no arguments
      if constexpr (std::is_void_v<R>) {
        func();
      } else {
        R r = func();
        packer.clear();
        packer.serialize(r);
        const auto serialized =
            std::vector<uint8_t>(packer.data(), packer.data() + packer.size());
        Serial.write(serialized.data(), serialized.size());
      }
    } else {
      // arguments, unpack them and call the function
      auto args = deserialize_as_tuple<Args...>(unpacker);

      if constexpr (std::is_void_v<R>) {
        std::apply(func, args);
      } else {
        R r = std::apply(func, args);
        // manage result here
        packer.clear();
        packer.serialize(r);
        const auto serialized =
            std::vector<uint8_t>(packer.data(), packer.data() + packer.size());
        Serial.write(serialized.data(), serialized.size());
      }
    }
    Serial.flush();
  }
};

void dispatch(const String name) {
  auto it = internals::rpc_table.find(name);
  if (it == internals::rpc_table.end()) {
    // TBD error
    return;
  }

  it->second.handler(it->second.func);
}

} // namespace internals

template <typename R, typename... Args>
void register_rpc(const String name, R (*func)(Args...),
                  const __FlashStringHelper *description) {
  internals::RpcEntry entry;
  entry.func = reinterpret_cast<void *>(func);
  entry.handler = &pyrpc::internals::RpcWrapper<R, Args...>::call;
  entry.description = description;

  internals::rpc_table[name] = entry;
}

inline void begin() {
  struct Help {
    static String call() {
      String descr;
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
    String call;
  };

  if (!Serial.available()) {
    return;
  }

  std::vector<uint8_t> rcv;
  RpcRequest request;
  // read the serial
  while (Serial.available()) {
    rcv.push_back(Serial.read());
  }

  // clear the unpacker, this is done only here to maintain its internal state
  internals::unpacker.clear();
  internals::unpacker.feed(rcv.data(), rcv.size());
  internals::unpacker.deserialize(request.call);
  internals::dispatch(request.call);
}

} // namespace pyrpc