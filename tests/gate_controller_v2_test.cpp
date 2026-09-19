#include "gate_control_sim.h"
bool ok(const Sim &s) { return std::string(s.control.status()).find("error_")!=0; }
void unknown(Sim &s,int pos,int last) {
  s.prepare(pos,last);
  s.tracker=PositionTracker{}; s.encoder_count=0;
  s.run(3000); s.origin=s.now;
  assert(!s.tracker.calibrated() && s.tracker.last_direction()==0);
}
int main() {
  std::cout << std::unitbuf;
  unsigned cases=0;
  for(int d:{-1,1}) for(uint32_t gap:{0U,50U,500U,1950U,2000U,2500U,5000U,22000U}) {
    Sim s; if(d==-1) s.prepare(1049,1);
    assert(s.request(d)); s.run(gap); assert(s.request(d)); s.run(25000);
    assert(s.rejected==0 && s.pulses.size()==1 && s.pos==(d==1?1049:0) && ok(s));
    s.summary(("same_target_"+std::to_string(d)+"_gap_"+std::to_string(gap)).c_str()); ++cases;
  }
  for(int d:{-1,1}) for(uint32_t gap:{0U,50U,100U,500U,1000U,1950U,2000U,2500U,5000U}) {
    Sim s; if(d==-1) s.prepare(1049,1);
    s.request(d); s.run(gap); assert(s.request(-d)); s.run(30000);
    assert(s.rejected==0 && s.pos==(d==1?0:1049) && ok(s));
    assert(s.pulses.size()==(gap==0?0U:3U));
    s.summary(("latest_target_"+std::to_string(-d)+"_gap_"+std::to_string(gap)).c_str()); ++cases;
  }
  for(int target:{-1,1}) for(int last:{-1,1}) {
    Sim s; s.prepare(500,last); s.request(target); s.run(10000);
    assert(s.pulses.size()==(target==last?3U:1U) && ok(s));
    assert(s.physical==target || s.pos==(target==1?1049:0));
    s.summary(("stopped_last_"+std::to_string(last)+"_target_"+std::to_string(target)).c_str()); ++cases;
  }
  for(int p:{20,100,990,1029}) {
    int target=p<500?1:-1;
    Sim s; s.prepare(p,target); s.request(target); s.run(10000);
    assert(s.physical==target && s.pulses.size()==2 && ok(s));
    s.summary(("endpoint_race_fixed_"+std::to_string(p)).c_str()); ++cases;
  }
  {
    Sim s; s.prepare(30,-1,-1); s.run(2000); assert(s.pos==0);
    s.request(-1); s.run(4000); assert(s.pulses.empty() && s.pos==0 && ok(s));
    s.summary("closed_settling_close_no_reopen"); ++cases;
  }
  for(uint32_t retarget:{500U,2500U,4200U}) {
    Sim s; s.prepare(500,1); s.request(1); s.run(retarget); s.request(-1); s.run(15000);
    assert(s.rejected==0 && s.pos==0 && ok(s));
    s.summary(("three_step_retarget_close_"+std::to_string(retarget)).c_str()); ++cases;
  }
  {
    Sim s; s.request(1); s.run(500); s.request(-1); s.run(500); s.request(1); s.run(25000);
    assert(s.pulses.size()==1 && s.pos==1049 && ok(s));
    s.summary("open_close_open_before_stop"); ++cases;
  }
  {
    Sim s; s.request(1); s.run(500); s.request(-1); s.run(2000); s.request(1); s.run(30000);
    // Reverse leg reaches closed endpoint before its planned stop: replan there.
    assert(s.pulses.size()==4 && s.pos==1049 && ok(s));
    s.summary("open_close_open_after_stop_issued"); ++cases;
  }
  {
    Sim s; s.prepare(500,1); s.request(1); s.run(500); s.raw(); s.run(6000);
    assert(s.pulses.size()==2 && s.physical==0 && !s.control.busy());
    s.summary("raw_press_queued_not_dropped"); ++cases;
  }
  {
    Sim s; s.request(1); s.run(5000); s.external_press(); s.run(500); s.request(1); s.run(9000);
    assert(s.pulses.size()==4 && s.physical==1 && ok(s));
    s.summary("external_stop_settling_resume"); ++cases;
  }
  for(int target:{-1,1}) for(int last:{-1,1}) {
    Sim s; unknown(s,500,last); s.allow_probe=true; s.request(target); s.run(10000);
    assert(s.pulses.size()==(target==last?3U:1U) && ok(s));
    assert(s.physical==target || s.pos==(target==1?1049:0));
    s.summary(("unknown_probe_last_"+std::to_string(last)+"_target_"+std::to_string(target)).c_str()); ++cases;
  }
  {
    Sim s; unknown(s,500,1); s.request(1); s.run(1000);
    assert(s.pulses.empty() && std::string(s.control.status())=="needs_direction_reference");
    s.summary("unknown_probe_not_authorized_explicit_status"); ++cases;
  }
  {
    Sim s; unknown(s,500,1); s.physical=1; s.run(1000); s.request(-1); s.run(12000);
    assert(s.pulses.size()==2 && s.pos==0 && ok(s));
    s.summary("uncalibrated_known_live_direction_works"); ++cases;
  }
  {
    Sim s; unknown(s,500,1); s.allow_probe=true; s.ignore_presses=true; s.request(1); s.run(10000);
    assert(s.pulses.size()==1 && std::string(s.control.status())=="error_no_motion");
    s.summary("probe_no_feedback_no_retry"); ++cases;
  }
  {
    Sim s; s.prepare(500,1); s.request(1); s.run(500); s.position_known=false; s.run(5000);
    assert(s.pulses.size()==1 && std::string(s.control.status())=="error_encoder_range");
    s.summary("encoder_fault_does_not_enable_probe"); ++cases;
  }
  {
    Sim s; s.prepare(800,1); s.request(-1); s.run(3000); s.physical=1; s.last_physical=1; s.run(6000);
    assert(s.pulses.size()==1); s.summary("obstacle_reversal_not_reclosed"); ++cases;
  }
  {
    Sim s; s.prepare(500,1); s.request(1); s.run(500); s.control=GateController{};
    s.tracker=PositionTracker{}; s.encoder_count=0; s.run(10000);
    assert(s.pulses.size()==1 && !s.control.busy()); s.summary("reboot_no_automatic_resume"); ++cases;
  }
  {
    Sim s; s.prepare(500,1); s.request(1); s.run(500); s.contact_available=false; s.run(9000);
    assert(s.pulses.size()==3 && s.physical==1 && ok(s)); s.summary("network_contact_loss_counting_continues"); ++cases;
  }
  {
    Sim s; s.request(1); for(int i=0;i<40;++i) {s.run(100);s.request(1);} s.run(25000);
    assert(s.pulses.size()==1 && s.pos==1049); s.summary("open_spam_40_no_extra_pulses"); ++cases;
  }
  {
    GateController c; GateObservation o{false,0,0,0,0};
    assert(c.request(1,0xffffff00U,false,true)); assert(c.tick(o,0xffffff00U,false));
    assert(!c.tick(o,0x1300U,false) && !c.busy());
    assert(std::string(c.status())=="error_no_motion"); ++cases;
  }
  std::cout<<"PASS "<<cases<<" v2 cases\n";
}
