#include <ares/ares.hpp>
#include "m24cxx.hpp"

namespace ares {

#include "serialization.cpp"

auto M24Cxx::power(Type typeID, n3 enableID) -> void {
  type     = typeID;
  mode     = Mode::Standby;
  clock    = {};
  data     = {};
  enable   = enableID;
  counter  = 0;
  device   = 0;
  address  = 0;
  input    = 0;
  output   = 0;
  response = Acknowledge;
  writable = 1;
}

auto M24Cxx::read() -> n1 {
  if(mode == Mode::Standby) return data.line;
  return response;
}

auto M24Cxx::write(maybe<n1> scl, maybe<n1> sda) -> void {
  auto phase = mode;
  clock.write(scl(clock.line));
  data.write(sda(data.line));

  if(clock.hi) {
    if(data.fall) counter = 0, mode = Mode::Device;
    if(data.rise) counter = 0, mode = Mode::Standby;
  }

  if(clock.fall) {
    if(counter++ > 8) counter = 1;
  }

  if(clock.rise)
  switch(phase) {
  case Mode::Device:
    if(counter <= 8) {
      device = device << 1 | data.line;
    } else if(!select()) {
      mode = Mode::Standby;
    } else if(device & 1) {
      mode = Mode::Read;
      response = load();
    } else {
      mode = Mode::Address;
      response = Acknowledge;
    }
    break;

  case Mode::Address:
    if(counter <= 8) {
      address = address << 1 | data.line;
    } else {
      mode = Mode::Write;
      response = Acknowledge;
    }
    break;

  case Mode::Read:
    if(counter <= 8) {
      response = output >> 8 - counter;
    } else if(data.line == Acknowledge) {
      address++;
      response = load();
    } else {
      mode = Mode::Standby;
    }
    break;

  case Mode::Write:
    if(counter <= 8) {
      input = input << 1 | data.line;
    } else {
      response = store();
      address++;
    }
    break;
  }
}

auto M24Cxx::select() -> bool {
  if(device >> 4 != 0b1010) return !Acknowledge;
  n3 mask = size() - 1 >> 8 ^ 7;
//if((device >> 1 & mask) != (enable & mask)) return !Acknowledge;
  return Acknowledge;
}

auto M24Cxx::load() -> bool {
  output = memory[(device >> 1 << 8 | address) & size() - 1];
  return Acknowledge;
}

auto M24Cxx::store() -> bool {
  if(!writable) return !Acknowledge;
  memory[(device >> 1 << 8 | address) & size() - 1] = input;
  return Acknowledge;
}

auto M24Cxx::erase(n8 fill) -> void {
  for(auto& byte : memory) byte = fill;
}

auto M24Cxx::Line::write(n1 data) -> void {
  lo   = !line && !data;
  hi   =  line &&  data;
  fall =  line && !data;
  rise = !line &&  data;
  line =  data;
}

}