"""Generate the deployable HA templates from the tested event reducer."""
from pathlib import Path
import yaml

ROOT = Path(__file__).resolve().parent
TRACKER = "sensor.linear_garage_statistics"
DATA = "(state_attr('sensor.linear_garage_statistics', 'data') or {})"


class Dumper(yaml.SafeDumper):
    pass


def represent_string(dumper, value):
    return dumper.represent_scalar("tag:yaml.org,2002:str", value, style="|" if "\n" in value else None)


Dumper.add_representer(str, represent_string)


def sensor(suffix, name, state, **options):
    return dict(name=f"Linear Garage {name}", unique_id=f"linear_garage_{suffix}",
                default_entity_id=f"sensor.linear_garage_{suffix}", state=state, **options)


def generate():
    triggers = [dict(trigger="state", entity_id=entity, id=kind, **{"to": None}) for kind, entity in (
        ("motion", "sensor.linear_garage_controller_motion"),
        ("gate", "sensor.linear_garage_controller_gate_state"),
        ("online", "binary_sensor.linear_garage_controller_controller_online"),
        ("contact", "binary_sensor.garage_closed_contact"),
    )]
    triggers += [dict(trigger="time_pattern", minutes="/1", id="tick"),
                 dict(trigger="homeassistant", event="start", id="startup"),
                 dict(trigger="event", event_type="event_template_reloaded", id="reload")]
    tracker = sensor("statistics", "Gate Statistics",
        "{{ 'monitoring' if is_state('binary_sensor.linear_garage_controller_controller_online', 'on') else 'monitor_offline' }}",
        icon="mdi:chart-timeline-variant", attributes={"data": (ROOT / "gate_statistics_data.jinja").read_text()})
    available = "{{ state_attr('sensor.linear_garage_statistics', 'data') is mapping }}"
    derived = []
    for direction, label in [("up", "Opening"), ("down", "Closing")]:
        for period, display in [("today", "Today"), ("total", "Total")]:
            key = f"{direction}_{period}"
            state = "{{ " + DATA + f".get('{key}', 0)" + " }}"
            if period == "today":
                state = "{{ " + DATA + f".get('{key}', 0) if " + DATA + ".get('date') == now().strftime('%Y-%m-%d') else 0 }}"
            derived.append(sensor(f"{label.lower()}_starts_{period}", f"Gate {label} Starts {display}", state,
                availability=available, state_class="total_increasing", unit_of_measurement="starts",
                icon="mdi:counter", attributes={
                    "meaning": "Observed movement starts, including manual controls and mid-travel resumes; offline movements are not inferred.",
                    "statistics_since": "{{ " + DATA + ".get('started_at') | timestamp_local }}"}))
    for direction in ["open", "close"]:
        derived.append(sensor(f"last_{direction}_duration", f"Gate Last {direction.title()} Duration",
            "{{ " + DATA + f".get('last_{direction}_s')" + " }}",
            availability="{{ " + DATA + f".get('last_{direction}_s') is number" + " }}",
            device_class="duration", unit_of_measurement="s", icon="mdi:timer-outline",
            attributes={"meaning": "Last uninterrupted full trip observed by HA; includes motion-stop detection and network reporting delay."}))
        derived.append(sensor(f"{direction}_duration_baseline", f"Gate {direction.title()} Duration Baseline",
            "{% set v = " + DATA + f".get('{direction}_samples', []) | sort " + "%}\n"
            "{% set n = v | length %}\n{{ ((v[(n-1)//2] + v[n//2])/2) | round(2) if n else none }}",
            availability="{{ " + DATA + f".get('{direction}_samples', []) | length > 0" + " }}",
            device_class="duration", unit_of_measurement="s", icon="mdi:timer-check-outline",
            attributes={"sample_count": "{{ " + DATA + f".get('{direction}_samples', []) | length" + " }}",
                        "alert_ready": "{{ " + DATA + f".get('{direction}_samples', []) | length >= 5" + " }}",
                        "meaning": "Median of up to ten accepted full trips in this direction; slow outliers excluded after learning."}))
    derived.append(sensor("gate_open_duration", "Gate Open Duration",
        "{% set d = " + DATA + " %}\n"
        "{{ ((as_timestamp(now()) - d.get('open_since', as_timestamp(now()))) / 60) | round(1) "
        "if is_state('binary_sensor.garage_closed_contact', 'on') and d.get('open_since', 0) > 0 else 0 }}",
        availability="{{ states('binary_sensor.garage_closed_contact') in ['on', 'off'] and "
                     "state_attr('sensor.linear_garage_statistics', 'data') is mapping }}",
        device_class="duration", unit_of_measurement="min", icon="mdi:garage-open",
        attributes={"estimated_lower_bound": "{{ " + DATA + ".get('open_since_estimated', true) }}",
                    "source": "binary_sensor.garage_closed_contact",
                    "meaning": "Time since leaving the closed contact, including partial opening; after unknown/startup only observed time is counted."}))
    content = [dict(triggers=triggers, sensor=[tracker]), dict(sensor=derived)]
    header = "# Generated by build_statistics.py. HA-only; never actuates the garage door.\n"
    (ROOT / "linear_garage_statistics.yaml").write_text(header + yaml.dump(content, Dumper=Dumper, allow_unicode=True, sort_keys=False, width=110))
    automation = dict(id="linear_garage_slow_trip_alert", alias="Linear Garage Gate Slow Trip Alert",
        description="Notify the configured phone only when a newly completed full trip exceeds its learned directional baseline. No door commands or TTS.",
        triggers=[dict(trigger="state", entity_id=TRACKER, attribute="data")],
        conditions=[dict(condition="template", value_template="""{% set before = trigger.from_state.attributes.get('data', {}) if trigger.from_state else {} %}
{% set after = trigger.to_state.attributes.get('data', {}) if trigger.to_state else {} %}
{{ before is mapping and 'slow_seq' in before and after.get('slow_seq', 0) > before.get('slow_seq', 0)
   and 0 <= as_timestamp(now()) - after.get('last_slow', {}).get('at', 0) < 30 }}""")],
        actions=[dict(action="notify.mobile_app_your_phone", data={
            "title": "Garage Gate Travel Slower Than Usual",
            "message": """{% set r = trigger.to_state.attributes['data']['last_slow'] %}
车库大门本次{{ '开启' if r.direction == 'opening' else '关闭' }}耗时 {{ r.seconds | round(1) }} 秒，平时基准约 {{ r.baseline | round(1) }} 秒。请留意是否有异常阻力；此提醒包含状态检测和网络延迟，不能单独判断机械故障。""",
            "data": {"tag": "linear-garage-slow-trip", "group": "garage-gate"}})], mode="single")
    (ROOT / "linear_garage_statistics_package.yaml").write_text(header + yaml.dump({"automation": [automation]}, Dumper=Dumper, allow_unicode=True, sort_keys=False, width=110))


if __name__ == "__main__":
    generate()
