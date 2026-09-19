#include "../firmware/position_tracker.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
using linear_garage::PositionTracker;
bool equal(const char *a, const char *b) { return std::strcmp(a, b) == 0; }
int main(int argc, char **argv) {
  PositionTracker t;
  t.sample(0, 0);
  assert(!t.calibrated() && std::isnan(t.percent()));
  assert(equal(t.state(3000), "unknown"));
  t.contact("off", 3000);
  assert(!t.tick(5499));
  assert(t.tick(5500) && t.percent() == 0 && equal(t.state(5500), "closed"));
  t.sample(-100, 6000); // old closed report must not zero an opening door
  assert(equal(t.state(6000), "opening"));
  assert(!t.tick(9000) && t.position_count() == 100);
  t.contact("on", 9000);
  t.sample(-1049, 10000);
  assert(t.percent() == 100);
  assert(equal(t.state(12000), "open_estimated"));
  t.contact("unavailable", 13000); // encoder continues independently
  t.sample(-500, 14000);
  assert(t.calibrated() && equal(t.motion(14000), "closing"));
  t.contact("off", 15000); // contradictory reconnect cannot re-zero mid-travel
  assert(!t.tick(18000) && t.position_count() == 500);
  t.contact("on", 18000);
  t.contact("off", 19000); // contact can close slightly before last motor step
  t.sample(-3, 19500);
  assert(!t.tick(21999));
  assert(t.tick(22000) && t.position_count() == 0);
  assert(t.calibrations() == 2);
  t.contact("off", 23000);
  assert(!t.tick(27000) && t.calibrations() == 2);

  PositionTracker reboot;
  reboot.sample(0, 0);
  reboot.contact("on", 100);
  reboot.sample(500, 1000); // booted midway, closing counts alone cannot locate zero
  assert(std::isnan(reboot.percent()));
  reboot.contact("unknown", 4000);
  assert(!reboot.tick(10000));
  reboot.contact("off", 11000);
  assert(reboot.tick(13500) && reboot.percent() == 0);
  reboot.sample(-2000, 14000); // implausible travel invalidates instead of clamping silently
  assert(!reboot.calibrated() && std::isnan(reboot.percent()));

  PositionTracker cancellation;
  cancellation.sample(0, 0);
  cancellation.contact("off", 1);
  cancellation.sample(-1, 1000);
  assert(!cancellation.tick(10000) && !cancellation.calibrated());
  cancellation.contact("on", 11000);
  cancellation.contact("off", 12000);
  cancellation.contact("unavailable", 13000);
  assert(!cancellation.tick(20000));

  PositionTracker wrap;
  wrap.sample(0, UINT32_MAX - 1000);
  wrap.contact("off", UINT32_MAX - 1000);
  assert(!wrap.tick(1000));
  assert(wrap.tick(2000));

  // Replay the actual field log; contact events here are simulated references,
  // not a claim that Aqara timing was captured with the original serial log.
  assert(argc == 2);
  std::ifstream f(argv[1]);
  assert(f.good());
  PositionTracker replay;
  replay.sample(0, 0);
  replay.contact("off", 0);
  assert(replay.tick(3000));
  std::string line;
  uint32_t now = 3500;
  int samples = 0;
  bool saw_open = false, saw_close = false;
  while (std::getline(f, line)) {
    auto pos = line.find("COUNT=");
    if (pos == std::string::npos) continue;
    int count = std::stoi(line.substr(pos + 6));
    if (count < 0) replay.contact("on", now);
    replay.sample(count, now);
    replay.tick(now);
    saw_open |= equal(replay.motion(now), "opening");
    saw_close |= equal(replay.motion(now), "closing");
    if (count == -1049) assert(replay.percent() == 100);
    assert(replay.calibrated());
    now += 500;
    ++samples;
  }
  assert(samples == 301 && saw_open && saw_close);
  assert(replay.position_count() == 0 && replay.percent() == 0);
  replay.contact("off", now);
  assert(replay.tick(now + 2500));
  assert(equal(replay.state(now + 2500), "closed"));
  std::cout << "PASS: startup, calibration settling, sign, motion, stale closed, reconnect, "
               "unknown reference, range fault, millis rollover; field replay " << samples << " samples\n";
}
