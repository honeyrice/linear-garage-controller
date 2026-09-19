#pragma once
// Deterministic physical model for the v2 controller and tracker.
// Motor model: one pulse stops motion; stopped pulse goes opposite last travel;
// physical endpoints override history. No hardware is accessed.
#include "../firmware/gate_controller.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
using namespace linear_garage;

struct Sim {
  PositionTracker tracker;
  GateController control;
  uint32_t now=3000, origin=3000, last_pulse=0, begin_at=0;
  int pos=0, physical=0, last_physical=0, pending_direction=0;
  int encoder_count=0, speed=3, start_delay=0;
  bool has_pulse=false, ignore_presses=false, lose_encoder=false, contact_available=true;
  bool position_known=true, allow_probe=false;
  unsigned accepted=0, rejected=0;
  std::vector<uint32_t> pulses;
  std::vector<std::string> trace;
  Sim() { tracker.sample(0,0); tracker.contact("off",0); tracker.tick(now); }
  bool pulse_busy() const { return has_pulse && uint32_t(now-last_pulse)<2000; }
  void note(const std::string &s) { trace.push_back(std::to_string(now-origin)+"ms: "+s); }
  void external_press() {
    if (physical || pending_direction) { physical=0; pending_direction=0; }
    else {
      const int d=pos==0 ? 1 : pos==1049 ? -1 : -last_physical;
      if (start_delay) { pending_direction=d; begin_at=now+start_delay; }
      else { physical=d; last_physical=d; }
    }
  }
  void pulse() {
    has_pulse=true; last_pulse=now; pulses.push_back(now-origin);
    note("PULSE "+std::to_string(pulses.size()));
    if (!ignore_presses) external_press();
  }
  bool request(int d) {
    bool result=control.request(d,now,pulse_busy(),allow_probe);
    result ? ++accepted : ++rejected;
    note(std::string(d==1 ? "OPEN " : "CLOSE ")+(result ? "accepted" : "ignored"));
    return result;
  }
  void raw() { control.request_raw(now); }
  void step() {
    now+=50;
    if (pending_direction && now>=begin_at) { physical=pending_direction; last_physical=physical; pending_direction=0; }
    int previous=pos;
    if (physical) {
      pos=std::clamp(pos+speed*physical,0,1049);
      if (pos==0 || pos==1049) physical=0;
    }
    if (!lose_encoder) encoder_count+=pos-previous;
    tracker.sample(-encoder_count,now);
    tracker.contact(contact_available ? (pos==0 ? "off" : "on") : "unavailable",now);
    // Match position.yaml: tracker.tick every 250ms; control every 50ms.
    if (now%250==0) tracker.tick(now);
    auto obs=observe_gate(tracker,now);
    if (!position_known) obs.position_fault=true;
    std::string old=control.status();
    if (control.tick(obs,now,pulse_busy())) pulse();
    if (old!=control.status()) note(control.status());
  }
  void run(uint32_t ms) { for (uint32_t i=0;i<ms;i+=50) step(); }
  void prepare(int p,int previous_direction,int moving=0) {
    // Seed calibrated real tracker with observed travel, never inject controller observations.
    if (previous_direction==1) {
      pos=p-3; encoder_count=pos; tracker.sample(-encoder_count,now+50);
      pos=p; encoder_count=p; tracker.sample(-encoder_count,now+100);
    } else {
      pos=p+3; encoder_count=pos; tracker.sample(-encoder_count,now+50);
      pos=p; encoder_count=p; tracker.sample(-encoder_count,now+100);
    }
    now+=100; physical=0; last_physical=previous_direction;
    tracker.contact(pos==0 ? "off" : "on",now);
    run(3000);
    physical=moving;
    if (moving) run(100);
    origin=now;
  }
  void summary(const char *name) const {
    std::cout<<name<<" | pulses="<<pulses.size()<<" | accepted="<<accepted<<" | ignored="<<rejected
             <<" | physical="<<physical<<" | position="<<pos<<" | state="<<tracker.state(now)
             <<" | result="<<control.status()<<"\n";
    for (const auto &s:trace) std::cout<<"  "<<s<<"\n";
  }
};
