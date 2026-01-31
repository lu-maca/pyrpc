# pyrpc

A tiny RPC bridge between Arduino and Python using MsgPack over serial.

It lets you expose C++ functions on an Arduino board and call them directly from Python, with automatic (de)serialization of basic types, containers, and structs.

## Arduino side

1. Install the Arduino MsgPack library (e.g. via Library Manager).
2. Include the RPC header and register your functions:

```cpp
#include <pyrpc.h>

double sum(int i, float d) {
	return i + d;
}

void setup() {
	Serial.begin(115200);

	pyrpc::begin();  

	pyrpc::register_rpc(
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
pip install pyrpc
```

Call your Arduino procedures from Python:

```python
from pyrpc import Rpc

rpc = Rpc("/dev/ttyACM0", baudrate=115200)

print(rpc.help)  # shows all available remote procedures

result = rpc.call("sum", 3, 1.5)
print("sum result:", result)

rpc.close()
```

A more complete example is available in the `examples/` folder.
