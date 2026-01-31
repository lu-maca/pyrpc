from ardupyrpc import Rpc

pyrpc = Rpc("/dev/ttyACM0")

print(pyrpc.help)

result = pyrpc.call("g")
print("RPC result:", result)

result = pyrpc.call("w", 3, 1.23)
print("RPC result:", result)

result = pyrpc.call("l")
print("RPC result:", result)

result = pyrpc.call("f")
print("RPC result:", result)

result = pyrpc.call("complex_struct", [45,21, -4])
print("RPC result:", result)

result = pyrpc.call("prod_int", 7, 6)
print("RPC result:", result)