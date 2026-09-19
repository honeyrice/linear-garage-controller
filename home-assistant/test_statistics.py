"""Replay the exact deployed Jinja reducer without touching live HA entities."""
import ast
import unittest
from datetime import datetime, timedelta
from pathlib import Path
from types import SimpleNamespace
from zoneinfo import ZoneInfo

import yaml
from jinja2.sandbox import ImmutableSandboxedEnvironment

ROOT = Path(__file__).resolve().parent


class Replay:
    def __init__(self):
        self.time = datetime(2026, 9, 18, 12, 0, tzinfo=ZoneInfo("America/Los_Angeles"))
        self.values = {
            "sensor.linear_garage_controller_motion": "stopped",
            "sensor.linear_garage_controller_gate_state": "closed",
            "binary_sensor.linear_garage_controller_controller_online": "on",
            "binary_sensor.garage_closed_contact": "off",
        }
        self.data = {}
        self.env = ImmutableSandboxedEnvironment()
        self.env.globals.update(now=lambda: self.time, as_timestamp=lambda v: v.timestamp(),
                                states=lambda entity: self.values.get(entity, "unknown"),
                                is_state=lambda entity, state: self.values.get(entity) == state)
        config = yaml.safe_load((ROOT / "linear_garage_statistics.yaml").read_text())
        self.template = self.env.from_string(config[0]["sensor"][0]["attributes"]["data"])
        self.event("reload")
        self.event("tick")

    def event(self, kind="tick", seconds=0, **values):
        self.time += timedelta(seconds=seconds)
        for k, v in values.items():
            key = {"motion": "sensor.linear_garage_controller_motion", "gate": "sensor.linear_garage_controller_gate_state",
                   "online": "binary_sensor.linear_garage_controller_controller_online", "contact": "binary_sensor.garage_closed_contact"}[k]
            self.values[key] = v
        self.data = ast.literal_eval(self.template.render(this=SimpleNamespace(attributes={"data": self.data}), trigger=SimpleNamespace(id=kind)).strip())
        return self.data

    def trip(self, direction, seconds=10, gate_first=False):
        if gate_first:
            self.event("gate", gate=direction)
            self.event("motion", motion=direction)
        else:
            self.event("motion", motion=direction)
            self.event("gate", gate=direction)
        self.event("contact", contact="on")
        self.event("motion", seconds=seconds, motion="stopped")
        self.event("gate", seconds=1, gate="open_estimated" if direction == "opening" else "closed")
        if direction == "closing":
            self.event("contact", contact="off")


class StatisticsTests(unittest.TestCase):
    def test_full_cycle_and_delayed_closed_confirmation(self):
        r = Replay(); r.trip("opening", 10); r.trip("closing", 12)
        self.assertEqual((r.data["up_today"], r.data["down_today"]), (1, 1))
        self.assertEqual((r.data["last_open_s"], r.data["last_close_s"]), (10, 12))
        self.assertEqual(r.data["open_since"], 0)

    def test_gate_motion_publication_order(self):
        r = Replay(); r.trip("opening", 10, gate_first=True); r.trip("closing", 12, gate_first=True)
        self.assertEqual((r.data["last_open_s"], r.data["last_close_s"]), (10, 12))

    def test_duplicate_updates_do_not_count(self):
        r = Replay(); r.event("motion", motion="opening"); r.event("gate", gate="opening")
        for _ in range(5): r.event("motion", seconds=1)
        self.assertEqual(r.data["up_total"], 1)

    def test_start_immediately_after_reload_is_observed(self):
        r = Replay(); r.event('reload'); r.trip('opening', 10)
        self.assertEqual(r.data['up_total'], 1)
        self.assertEqual(r.data['last_open_s'], 10)

    def test_midtravel_stop_resume_excluded_from_full_trip(self):
        r = Replay(); r.event("motion", motion="opening"); r.event("gate", gate="opening")
        r.event("motion", seconds=4, motion="stopped"); r.event("gate", gate="stopped")
        r.event("motion", seconds=1, motion="opening"); r.event("gate", gate="opening")
        r.event("motion", seconds=5, motion="stopped"); r.event("gate", gate="open_estimated")
        self.assertEqual(r.data["up_total"], 2)
        self.assertIsNone(r.data["last_open_s"])

    def test_reversal_excluded_and_both_directions_counted(self):
        r = Replay(); r.event("motion", motion="opening"); r.event("gate", gate="opening")
        r.event("motion", seconds=3, motion="closing"); r.event("gate", gate="closing")
        r.event("motion", seconds=3, motion="stopped"); r.event("gate", gate="closed")
        self.assertEqual((r.data["up_total"], r.data["down_total"]), (1, 1))
        self.assertEqual(r.data["close_samples"], [])

    def test_disconnect_and_reconnect_midtravel(self):
        r = Replay(); r.event("motion", motion="opening"); r.event("gate", gate="opening")
        r.event("online", seconds=3, online="off"); r.event("online", seconds=2, online="on")
        r.event("motion", seconds=5, motion="stopped"); r.event("gate", gate="open_estimated")
        self.assertEqual(r.data["up_total"], 1)
        self.assertIsNone(r.data["last_open_s"])

    def test_unknown_position_invalidates_trip(self):
        r = Replay(); r.event("motion", motion="opening"); r.event("gate", gate="opening")
        r.event("gate", seconds=3, gate="unknown"); r.event("gate", gate="opening")
        r.event("motion", seconds=5, motion="stopped"); r.event("gate", gate="open_estimated")
        self.assertIsNone(r.data["last_open_s"])

    def test_restart_preserves_totals_not_incomplete_trip(self):
        r = Replay(); r.trip("opening"); r.event("motion", motion="closing"); r.event("gate", gate="closing")
        r.event("startup", seconds=3); r.event("motion", seconds=4, motion="stopped"); r.event("gate", gate="closed")
        self.assertEqual((r.data["up_total"], r.data["down_total"]), (1, 1))
        self.assertEqual(r.data["last_open_s"], 10)
        self.assertIsNone(r.data["last_close_s"])

    def test_midnight_resets_today_only(self):
        r = Replay(); r.trip("opening"); r.event(seconds=86400)
        self.assertEqual((r.data["up_today"], r.data["up_total"]), (0, 1))

    def test_midnight_during_trip_preserves_timing(self):
        r = Replay(); r.time = r.time.replace(hour=23, minute=59, second=55)
        r.trip("opening", 10)
        self.assertEqual(r.data["up_today"], 0)
        self.assertEqual(r.data["last_open_s"], 10)

    def test_training_then_slow_alert_and_baseline_not_polluted(self):
        r = Replay()
        for _ in range(5): r.trip("opening", 10); r.trip("closing", 12)
        self.assertEqual(r.data["slow_seq"], 0)
        r.trip("opening", 15)
        self.assertEqual(r.data["slow_seq"], 1)
        self.assertEqual(r.data["last_slow"]["baseline"], 10)
        self.assertEqual(r.data["open_samples"], [10] * 5)
        r.event("reload"); r.event("tick")
        self.assertEqual(r.data["slow_seq"], 1)

    def test_threshold_boundary_and_directional_baselines(self):
        r = Replay()
        for _ in range(5): r.trip("opening", 10); r.trip("closing", 20)
        r.trip("opening", 13); r.trip("closing", 25)
        self.assertEqual(r.data["slow_seq"], 0)
        r.trip("opening", 13.1)
        self.assertEqual(r.data["slow_seq"], 1)

    def test_contact_timer_independent_of_esp32(self):
        r = Replay(); r.event("online", online="off"); r.event("contact", contact="on")
        start = r.data["open_since"]
        r.event(seconds=600)
        self.assertEqual(r.data["open_since"], start)
        self.assertFalse(r.data["open_since_estimated"])
        r.event("contact", contact="unavailable"); r.event("contact", seconds=60, contact="on")
        self.assertGreater(r.data["open_since"], start)
        self.assertTrue(r.data["open_since_estimated"])

    def test_startup_open_timer_becomes_lower_bound(self):
        r = Replay(); r.event("contact", contact="on"); start = r.data["open_since"]
        r.event("startup", seconds=600)
        self.assertGreater(r.data["open_since"], start)
        self.assertTrue(r.data["open_since_estimated"])

    def test_sample_history_bounded(self):
        r = Replay()
        for _ in range(14): r.trip("opening"); r.trip("closing")
        self.assertEqual(len(r.data["open_samples"]), 10)
        self.assertEqual(len(r.data["close_samples"]), 10)

    def test_only_notification_service_in_new_package(self):
        data = yaml.safe_load((ROOT / "linear_garage_statistics_package.yaml").read_text())
        self.assertEqual([a["action"] for a in data["automation"][0]["actions"]], ["notify.mobile_app_your_phone"])

    def test_notification_condition_does_not_replay_restored_alert(self):
        r = Replay()
        config = yaml.safe_load((ROOT / "linear_garage_statistics_package.yaml").read_text())
        condition = r.env.from_string(config['automation'][0]['conditions'][0]['value_template'])
        def check(before, after):
            trigger = SimpleNamespace(from_state=SimpleNamespace(attributes=before), to_state=SimpleNamespace(attributes=after))
            return condition.render(trigger=trigger).strip() == 'True'
        data = dict(slow_seq=1, last_slow=dict(at=r.time.timestamp()))
        self.assertFalse(check({}, dict(data=data)))
        self.assertFalse(check(dict(data=data), dict(data=data)))
        self.assertTrue(check(dict(data=dict(slow_seq=0)), dict(data=data)))
        r.time += timedelta(seconds=31)
        self.assertFalse(check(dict(data=dict(slow_seq=0)), dict(data=data)))

    def test_derived_sensors_render_learned_data(self):
        r = Replay(); r.trip('opening', 10); r.trip('closing', 12)
        r.env.globals['state_attr'] = lambda entity, attr: r.data
        r.env.filters['timestamp_local'] = lambda ts: datetime.fromtimestamp(ts, ZoneInfo('America/Los_Angeles')).isoformat()
        config = yaml.safe_load((ROOT / 'linear_garage_statistics.yaml').read_text())
        actual = {}
        for entity in config[1]['sensor']:
            self.assertEqual(r.env.from_string(entity['availability']).render().strip(), 'True')
            actual[entity['unique_id']] = r.env.from_string(entity['state']).render().strip()
            for value in entity.get('attributes', {}).values():
                if '{{' in value: r.env.from_string(value).render()
        self.assertEqual(float(actual['linear_garage_open_duration_baseline']), 10)
        self.assertEqual(float(actual['linear_garage_close_duration_baseline']), 12)
        self.assertEqual(float(actual['linear_garage_gate_open_duration']), 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
