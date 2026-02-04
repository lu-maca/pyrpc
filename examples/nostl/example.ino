#include "pyrpc.h"

struct MyStruct {
  int i;
  MsgPack::arr_t<int> v;
  MSGPACK_DEFINE( i, v);
};

// return a primitive type
double sum(int i, float d) {
    return (i + d);
}

// return containers
MsgPack::map_t<String, MsgPack::arr_t<int>> map_vector() {
  std::map<String, std::vector<int>> out {{"Ciao", {2,3}}, {"pepe", {3,4}}};
  return out;
}

// return a struct that uses MSGPACK_DEFINE
MyStruct my_struct() {
  MyStruct a{.i = 23, .v = {13,14,15}};
  return a;
}

// return a complex container that carries a struct
MsgPack::map_t<String, MyStruct> complex_struct(const MsgPack::arr_t<int> v_in) {
  MyStruct a {.i = 100, .v = v_in};
  MyStruct b {.i = -100, .v = v_in};
  
  MsgPack::map_t<String, MyStruct> out { {"a complex struct", a}, { "another one", b} };
  return out;
} 

// use function templates
template <typename T>
T prod(T a, T b) {
  return (a * b); 
}

void setup() {
  Serial.begin(115200);
  
  // begin the pyrpc library
  pyrpc::begin();

  pyrpc::registerRpc("sum", sum, F("@brief sum the two arguments @return double @arg i is an integer @arg d is a float"));
  pyrpc::registerRpc("my_struct", my_struct, F("@brief create an object and return it @return MyStruct"));
  pyrpc::registerRpc("map_vector", map_vector, F("@brief create a complex map and return it @return std::map<String, std::vector<int>> "));
  pyrpc::registerRpc("complex_struct", complex_struct, F("@brief A very complex struct @return std::map<String, MyStruct> @arg std::vector<int>"));
  pyrpc::registerRpc("prod_int", prod<int>, F("@brief Perform the product between two integers @return int @arg a @arg b"));
}

void loop() {
  pyrpc::process();
}

