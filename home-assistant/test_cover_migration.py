"""Render the production cover templates and inspect action routing; no HA calls."""
from pathlib import Path
import unittest
import yaml
from jinja2 import Environment

COVER = yaml.safe_load((Path(__file__).parent / 'garage_gate_controller.yaml').read_text())[0]['cover'][0]


class CoverTests(unittest.TestCase):
    def render(self, field, online='on', motion='stopped', gate='closed', position='0', button='unknown'):
        states = {
            'binary_sensor.linear_garage_controller_controller_online': online,
            'sensor.linear_garage_controller_motion': motion,
            'sensor.linear_garage_controller_gate_state': gate,
            'sensor.linear_garage_controller_opening': position,
            'button.linear_garage_controller_open_gate': button,
            'button.linear_garage_controller_close_gate': button,
        }
        env = Environment()
        env.globals.update(states=lambda entity: states.get(entity, 'unknown'), is_state=lambda entity, state: states.get(entity)==state)
        return env.from_string(field).render().strip()

    def test_cover_identity_preserved(self):
        self.assertEqual(COVER['unique_id'], 'garage_gate_controller')
        self.assertEqual(COVER['default_entity_id'], 'cover.garage_gate_controller')

    def test_offline_is_unavailable_even_with_cached_closed_position(self):
        self.assertEqual(self.render(COVER['availability'], online='off'), 'False')
        self.assertEqual(self.render(COVER['availability'], button='unavailable'), 'False')

    def test_never_pressed_buttons_are_still_usable(self):
        self.assertEqual(self.render(COVER['availability'], button='unknown'), 'True')

    def test_uncalibrated_position_does_not_block_directional_requests(self):
        self.assertEqual(self.render(COVER['availability'], gate='unknown', position='unknown'), 'True')
        self.assertEqual(self.render(COVER['position'], position='unknown'), 'None')
        self.assertEqual(self.render(COVER['state'], gate='unknown'), 'None')

    def test_movement_state_takes_precedence(self):
        for motion in ['opening', 'closing']:
            self.assertEqual(self.render(COVER['state'], motion=motion), motion)

    def test_endpoint_and_partial_open_mapping(self):
        self.assertEqual(self.render(COVER['state']), 'closed')
        for state in ['stopped', 'open_estimated']:
            self.assertEqual(self.render(COVER['state'], gate=state), 'open')
        self.assertEqual(self.render(COVER['position'], position='68.35'), '68')

    def test_open_close_route_to_directional_buttons_without_old_contact_gating(self):
        for action, entity in [('open_cover','open_gate'), ('close_cover','close_gate')]:
            self.assertEqual(COVER[action][-1], {'action':'button.press', 'target':{'entity_id':'button.linear_garage_controller_'+entity}})
            self.assertNotIn('garage_closed_contact', str(COVER[action]))
        self.assertNotIn('light.garage_door_opener', str(COVER))
        self.assertNotIn('set_cover_position', COVER)

    def test_stop_only_when_moving(self):
        guard = COVER['stop_cover'][1]['value_template']
        for state in ['stopped', 'unavailable', 'unknown']:
            self.assertEqual(self.render(guard, motion=state), 'False')
        for state in ['opening', 'closing']:
            self.assertEqual(self.render(guard, motion=state), 'True')
        self.assertEqual(COVER['stop_cover'][-1]['target']['entity_id'], 'button.linear_garage_controller_gate_button')


if __name__ == '__main__':
    unittest.main(verbosity=2)
