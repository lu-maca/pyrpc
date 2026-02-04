from ardupyrpc import Rpc

# Initialize the RPC client and send an onConnect message. The onConnect 
# will execute any procedure registered as onConnect on the server side.
# If no procedure is registered, the message is ignored.
pyrpc = Rpc("/dev/ttyACM0", on_connect=True)

print(pyrpc.help)

result = pyrpc.call("sum", 5, 2.5)
print("RPC result:", result)

result = pyrpc.call("my_struct")
print("RPC result:", result)

result = pyrpc.call("map_vector")
print("RPC result:", result)

result = pyrpc.call("complex_struct", [1,2,3,4])
print("RPC result:", result)

result = pyrpc.call("prod_int", 7, 6)
print("RPC result:", result)