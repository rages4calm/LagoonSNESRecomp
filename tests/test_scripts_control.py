"""Apply the existing strict-control contract suite to the new script service."""
import copy
import importlib.util
from pathlib import Path
import unittest
from unittest import mock

spec=importlib.util.spec_from_file_location('iterator_contract',Path(__file__).with_name('test_iterator_control.py'))
contract=importlib.util.module_from_spec(spec);spec.loader.exec_module(contract)
compare=contract.compare

def scripts(run):
    run=copy.deepcopy(run)
    for overrides in (run['receipt']['diagnostic_overrides'],run['normalized_overrides']):
        for key in list(overrides):
            if key.startswith('LAGOON_NATIVE_ITERATOR52'):
                overrides[key.replace('ITERATOR52','SCRIPTS53')]=overrides.pop(key)
    return run

class ScriptsControl(contract.IteratorControl):
    def helper(self,left=None,right=None):
        left=scripts(left or contract.run('baseline','off'))
        right=scripts(right or contract.run('candidate','auto'))
        return compare.scripts53_control(left['receipt'],right['receipt'],left['normalized_overrides'],right['normalized_overrides'])

    def full(self,left=None,right=None,**kwargs):
        left=scripts(left or contract.run('baseline','off'))
        right=scripts(right or contract.run('candidate','auto'))
        if 'native_iterator_control' in kwargs:
            kwargs['native_scripts_control']=kwargs.pop('native_iterator_control')
        return super().full(left,right,**kwargs)

    def test_mixed_controls_refuse_before_loading(self):
        for controls in [dict(native_scripts_control=True,native_iterator_control=True),
                         dict(native_scripts_control=True,interp_control_target='0280C7'),
                         dict(native_scripts_control=True,native_iterator_control=True,interp_control_target='0280C7')]:
            with self.subTest(controls=controls),mock.patch.object(compare,'load_run') as load:
                with self.assertRaisesRegex(ValueError,'exactly one'):
                    compare.compare_runs('baseline','candidate',**controls)
                load.assert_not_called()

    def test_iterator_switch_is_not_part_of_script_control(self):
        left=scripts(contract.run('baseline','off'));right=scripts(contract.run('candidate','auto'))
        left['normalized_overrides']['LAGOON_NATIVE_ITERATOR52']='off'
        right['normalized_overrides']['LAGOON_NATIVE_ITERATOR52']='auto'
        self.assertFalse(compare.scripts53_control(left['receipt'],right['receipt'],left['normalized_overrides'],right['normalized_overrides'])['approved'])

if __name__=='__main__':unittest.main()
