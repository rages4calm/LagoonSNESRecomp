"""Iterator52 changes one declared input; all observed parity remains strict.

The small in-memory fixtures exercise the real comparator without game ROMs,
private checkpoints, executable copies or filesystem cleanup. Run python -P.
"""
import importlib.util
import json
from pathlib import Path
import unittest
from unittest import mock

spec=importlib.util.spec_from_file_location('lagoon_iterator_comparator',Path(__file__).resolve().parents[1]/'tools/compare_native_runs.py')
compare=importlib.util.module_from_spec(spec);spec.loader.exec_module(compare)


def run(name,mode):
    receipt={key:'same-'+key for key in compare.INPUT_KEYS}
    receipt.update(exe_sha256='a'*64,program_manifest_sha256='b'*64,
                   frames=12,seeded_save_files=[],campaign_warps=None,
                   diagnostic_overrides={'LAGOON_NATIVE_ITERATOR52':mode,
                                         'LAGOON_NATIVE_ITERATOR52_REPORT':'1'})
    return dict(directory=Path(name),receipt=receipt,errors=[],tags=['cp'],
                normalized_overrides=dict(receipt['diagnostic_overrides']),
                captures={key:dict(relative_path='capture-'+str(i),size=4,sha256='c'*64)
                          for i,key in enumerate(sorted(compare.STRICT_CAPTURE_PATHS))},
                presentations=[dict(present='0',frame='1',alpha='0',crc32='1234',luma='5')],
                identity=dict(directory=name,executable_sha256='a'*64,manifest_sha256='b'*64))


class IteratorControl(unittest.TestCase):
    def helper(self,left=None,right=None):
        left=left or run('baseline','off');right=right or run('candidate','auto')
        return compare.iterator52_control(left['receipt'],right['receipt'],
            left['normalized_overrides'],right['normalized_overrides'])

    def full(self,left=None,right=None,component=None,clock=None,**kwargs):
        left=left or run('baseline','off');right=right or run('candidate','auto')
        info=[dict(frame=1,master_clock=123,vpos=4,hpos=5) for _ in range(2)]
        if clock:info[1][clock]+=1
        def component_result(a,b):
            return dict(exact=not(component and a.name=='cp.'+component))
        def read_text(path,*args,**kw):
            return json.dumps(info[1 if path.parts[0]=='candidate' else 0])
        with mock.patch.object(compare,'load_run',side_effect=[left,right]), \
             mock.patch.object(compare,'compare_component',side_effect=component_result), \
             mock.patch.object(Path,'is_file',return_value=True), \
             mock.patch.object(Path,'read_text',read_text):
            return compare.compare_runs('baseline','candidate',**kwargs)

    def test_only_explicit_off_to_auto_or_on_is_approved(self):
        for mode in ('auto','on'):
            with self.subTest(mode=mode):
                self.assertTrue(self.helper(right=run('candidate',mode))['approved'])
        for old,new in [('auto','on'),('on','auto'),('off','off'),('off','verify'),
                        ('off',None),(None,'auto'),('OFF','auto'),('off','AUTO')]:
            with self.subTest(old=old,new=new):
                self.assertFalse(self.helper(run('baseline',old),run('candidate',new))['approved'])

    def test_exact_executable_and_manifest_identities_are_required(self):
        for key in ('exe_sha256','program_manifest_sha256'):
            for value in ('d'*64,None,'a'*63,'A'*64,17):
                with self.subTest(key=key,value=value):
                    left,right=run('baseline','off'),run('candidate','auto')
                    right['receipt'][key]=value
                    self.assertFalse(self.helper(left,right)['approved'])
            for value in ('invalid',None,17):
                with self.subTest(key=key,baseline=value):
                    left,right=run('baseline','off'),run('candidate','auto')
                    left['receipt'][key]=right['receipt'][key]=value
                    self.assertFalse(self.helper(left,right)['approved'])

    def test_every_other_override_remains_exact(self):
        for change in ('changed_report','added','removed'):
            with self.subTest(change=change):
                left,right=run('baseline','off'),run('candidate','auto')
                if change=='changed_report':right['normalized_overrides']['LAGOON_NATIVE_ITERATOR52_REPORT']='0'
                elif change=='added':right['normalized_overrides']['LAGOON_OTHER']='on'
                else:right['normalized_overrides'].pop('LAGOON_NATIVE_ITERATOR52_REPORT')
                self.assertFalse(self.helper(left,right)['approved'])
                self.assertFalse(self.full(left,right,native_iterator_control=True)['passed'])

    def test_declared_full_comparator_control_accepts_but_keeps_difference_visible(self):
        for mode in ('auto','on'):
            with self.subTest(mode=mode):
                result=self.full(right=run('candidate',mode),native_iterator_control=True)
                self.assertTrue(result['passed'])
                self.assertTrue(result['execution_control']['approved'])
                self.assertIn('diagnostic_overrides',result['input_differences'])
                self.assertEqual(result['unapproved_input_differences'],{})
                self.assertEqual(len(result['checkpoints'][0]['components']),12)

    def test_mode_difference_without_declaration_is_rejected(self):
        result=self.full()
        self.assertFalse(result['passed'])
        self.assertIn('diagnostic_overrides',result['unapproved_input_differences'])

    def test_every_other_receipt_input_remains_strict(self):
        for key in set(compare.INPUT_KEYS)-{'diagnostic_overrides'}:
            with self.subTest(key=key):
                left,right=run('baseline','off'),run('candidate','auto')
                right['receipt'][key]=['different']
                result=self.full(left,right,native_iterator_control=True)
                self.assertFalse(result['passed'])
                self.assertIn(key,result['unapproved_input_differences'])

    def test_seed_and_campaign_warp_inputs_remain_strict(self):
        for key,value in [('seeded_save_files',[{'copy':'save0.sav','sha256':'e'*64}]),
                          ('campaign_warps',{'sha256':'f'*64})]:
            with self.subTest(key=key):
                left,right=run('baseline','off'),run('candidate','auto')
                right['receipt'][key]=value
                self.assertFalse(self.full(left,right,native_iterator_control=True)['passed'])

    def test_every_checkpoint_component_difference_still_fails(self):
        for component in (*compare.COMPONENTS,'info.json'):
            with self.subTest(component=component):
                result=self.full(component=component,native_iterator_control=True)
                self.assertFalse(result['passed'])
                self.assertFalse(result['checkpoints'][0]['components'][component]['exact'])

    def test_actual_clock_fields_are_independently_compared(self):
        for key in ('frame','master_clock','vpos','hpos'):
            with self.subTest(clock=key):
                result=self.full(clock=key,native_iterator_control=True)
                self.assertFalse(result['passed'])
                self.assertFalse(result['checkpoints'][0]['clock_exact'])

    def test_strict_coprocessor_capture_differences_still_fail(self):
        for key in compare.STRICT_CAPTURE_PATHS:
            for change in ('bytes','missing'):
                with self.subTest(key=key,change=change):
                    left,right=run('baseline','off'),run('candidate','auto')
                    if change=='bytes':right['captures'][key]['sha256']='d'*64
                    else:right['captures'].pop(key)
                    result=self.full(left,right,native_iterator_control=True)
                    self.assertFalse(result['passed'])
                    self.assertFalse(result['diagnostic_outputs'][key]['exact'])

    def test_presentation_tags_and_runtime_errors_still_fail(self):
        for change in ('presentation','missing_tag','extra_tag','baseline_error','candidate_error'):
            with self.subTest(change=change):
                left,right=run('baseline','off'),run('candidate','auto')
                if change=='presentation':right['presentations'][0]['crc32']='bad'
                elif change=='missing_tag':right['tags']=[]
                elif change=='extra_tag':right['tags'].append('other')
                elif change=='baseline_error':left['errors']=['fatal runtime error']
                else:right['errors']=['fatal runtime error']
                self.assertFalse(self.full(left,right,native_iterator_control=True)['passed'])

    def test_both_control_flags_are_rejected_before_loading(self):
        with mock.patch.object(compare,'load_run') as load:
            with self.assertRaisesRegex(ValueError,'exactly one'):
                compare.compare_runs('baseline','candidate',interp_control_target='0280C7',native_iterator_control=True)
            load.assert_not_called()


if __name__=='__main__':unittest.main()
