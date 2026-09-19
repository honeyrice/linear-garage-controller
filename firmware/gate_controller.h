#pragma once
#include <cstdint>
#include <cstring>
#include "position_tracker.h"

namespace linear_garage {
struct GateObservation {
  bool valid;
  int motion;          // +1 opening, -1 closing, 0 stopped
  int last_direction;  // encoder observation, never the last command
  int endpoint;        // +1 estimated open, -1 contact-confirmed closed
  int64_t count;
  uint32_t quiet_ms{0};
  bool closed_pending{false};
  bool position_fault{false};
};
inline GateObservation observe_gate(const PositionTracker &t, uint32_t now) {
  const char *m=t.motion(now), *s=t.state(now);
  return {t.calibrated(), std::strcmp(m,"opening")==0 ? 1 :
          std::strcmp(m,"closing")==0 ? -1 : 0, t.last_direction(),
          std::strcmp(s,"closed")==0 ? -1 :
          std::strcmp(s,"open_estimated")==0 ? 1 : 0, t.count(),
          t.quiet_ms(now), t.closed_pending(), t.position_fault()};
}

// Latest target wins, but an already-issued pulse is observed before replanning.
// No target is restored or executed on boot. Unknown-direction probing is opt-in.
class GateController {
 public:
  bool busy() const { return stage_!=Idle; }
  const char *status() const { return status_; }
  unsigned pulses() const { return pulses_; }
  int target() const { return target_; }
  void cancel() { if (busy()) finish("cancelled"); }
  bool request(int direction, uint32_t now, bool pulse_busy, bool allow_probe=false) {
    if (direction!=1 && direction!=-1) return false;
    allow_probe_=allow_probe;
    if (!busy()) {
      started_=now; pulses_=0; stage_=Decide;
    } else if (stage_==RawPending) {
      // A directional request can supersede a raw press not yet emitted.
      stage_=Decide;
    }
    const bool changed=target_!=direction;
    target_=direction;
    if (changed || stage_==Decide)
      status_=pulse_busy ? (target_==1 ? "open_queued" : "close_queued") :
                          (target_==1 ? "open_requested" : "close_requested");
    return true;  // opposite requests are accepted even during pulse cooldown
  }
  void request_raw(uint32_t now) {
    if (stage_!=RawPending) { started_=now; pulses_=0; }
    target_=0; stage_=RawPending; status_="single_press_queued";
  }
  // Return true once per pulse; caller provides the shared pulse/cooldown lock.
  bool tick(const GateObservation &o, uint32_t now, bool pulse_busy) {
    if (!busy()) return false;
    if (uint32_t(now-started_)>=20000) { finish("error_sequence_timeout"); return false; }
    if (stage_==RawPending) {
      if (pulse_busy) return false;
      ++pulses_; finish("single_press_sent"); return true;
    }
    if (o.position_fault) { finish("error_encoder_range"); return false; }
    if (stage_==WaitMotion) {
      if (o.count!=pulse_count_) {
        if (o.motion==0 || (expected_!=0 && o.motion!=expected_)) {
          finish("error_unexpected_motion"); return false;
        }
        // Replan using the newest target, including changes during the pulse.
        stage_=Decide;
        status_="motion_confirmed";
      } else if (uint32_t(now-pulse_at_)>=5000) finish("error_no_motion");
      return false;
    }
    if (stage_==WaitStop) {
      if (o.motion==0) { stage_=Decide; status_="stop_confirmed"; }
      else if (o.motion!=expected_) finish("error_unexpected_reversal");
      else if (uint32_t(now-pulse_at_)>=5000) finish("error_did_not_stop");
      return false;
    }
    // Tracker holds moving for 1500ms after last edge. Avoid a stop pulse using
    // stale motion near an endpoint or after an external stop.
    if (o.motion!=0 && o.quiet_ms>=250) { status_="waiting_motion_settle"; return false; }
    if (o.closed_pending && o.motion<=0) { status_="waiting_closed_reference"; return false; }
    if (o.endpoint==target_) {
      finish(target_==1 ? "already_open_estimated" : "already_closed"); return false;
    }
    if (o.motion==target_) {
      finish(target_==1 ? "opening_confirmed" : "closing_confirmed"); return false;
    }
    if (pulse_busy) { status_=target_==1 ? "open_queued" : "close_queued"; return false; }
    if (o.motion==-target_) return emit(WaitStop,o,now,o.motion,"waiting_stop");
    // Calibration is only needed for absolute endpoints, not live direction.
    const int next=o.endpoint!=0 ? -o.endpoint : -o.last_direction;
    if (next==0 && !allow_probe_) { finish("needs_direction_reference"); return false; }
    return emit(WaitMotion,o,now,next,next==0 ? "checking_direction" : "waiting_motion");
  }
 private:
  enum Stage { Idle, Decide, WaitMotion, WaitStop, RawPending };
  void finish(const char *s) { stage_=Idle; status_=s; }
  bool emit(Stage next,const GateObservation &o,uint32_t now,int expected,const char *s) {
    // Stable targets need <=3. Retargeting can add stop/start pairs; retain a
    // burst-wide bound even if the user keeps changing the requested target.
    if (pulses_>=6) { finish("error_pulse_limit"); return false; }
    ++pulses_; stage_=next; pulse_at_=now; pulse_count_=o.count;
    expected_=expected; status_=s; return true;
  }
  Stage stage_{Idle};
  const char *status_{"idle"};
  int target_{0}, expected_{0};
  unsigned pulses_{0};
  uint32_t started_{0},pulse_at_{0};
  int64_t pulse_count_{0};
  bool allow_probe_{false};
};
}  // namespace linear_garage
