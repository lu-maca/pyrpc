# pyrpc

A tiny RPC bridge between Arduino and Python using MsgPack over serial.

It lets you expose C++ functions on an Arduino board and call them directly from Python, with automatic (de)serialization of basic types, containers, and structs.

## Arduino side

Install `pyrpc` from the Arduino Library Manager.

```cpp
#include <pyrpc.h>

double sum(int i, float d) {
	return i + d;
}

void setup() {
	Serial.begin(115200);

	pyrpc::begin();  

	// register an onConnect procedure (called when Python connects)
	pyrpc::onConnect([]() {
		// do whatever you need here
	});

	pyrpc::registerRpc(
		"sum",
		sum,
		F("@brief sum two numbers @return double @arg i integer @arg d float")
	);
}

void loop() {
	pyrpc::process();  // handle incoming RPC calls
}
```

Upload this sketch to your Arduino (baudrate must match the Python side).

## Python side

Install the Python package:

```bash
pip install ardupyrpc
```

Call your Arduino procedures from Python:

```python
from ardupyrpc import Rpc

# on_connect=True sends an onConnect message to the Arduino after connecting
rpc = Rpc("/dev/ttyACM0", baudrate=115200, on_connect=True)

print(rpc.help)  # shows all available remote procedures

result = rpc.call("sum", 3, 1.5)
print("sum result:", result)

rpc.close()
```

A more complete example is available in the `examples/` folder.

## Current limitations
When STL containers are not supported, there is a limation of 128 bytes for the serialized data. This means that complex nested structures may not work as expected and the `help` command may fail to retrieve the full documentation.