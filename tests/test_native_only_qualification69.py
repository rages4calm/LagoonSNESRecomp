"""Prevent ignored policy flags or incomplete counters from certifying native execution."""
from pathlib import Path
import importlib.util
import unittest
spec=importlib.util.spec_from_file_location('qualify_native69',Path(__file__).resolve().parents[1]/'tools/qualify_native.py')
q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
cspec=importlib.util.spec_from_file_location('compare_native69',Path(__file__).resolve().parents[1]/'tools/compare_native_runs.py')
compare=importlib.util.module_from_spec(cspec);cspec.loader.exec_module(compare)
ARM='[native-only] armed schema=1 processor=main65816 strict=1\n'
SUMMARY='[native-only] summary schema=1 strict=1 interpreted_opcodes=0 completed_opcodes=0 runtime_invocations=0 unqualified_replay_activations=0 generic_accelerations_bypassed=17 blocked_events=0 bridge_uncovered_attempts=0 source_native_opcodes=123 interrupt_native_opcodes=45\n'
class NativeOnlyContract(unittest.TestCase):
    def test_default_control_not_promoted(self):self.assertIsNone(q.native_only_evidence(ARM+SUMMARY,False))
    def test_positive_guard_and_zero_counters(self):self.assertTrue(q.native_only_evidence(ARM+SUMMARY,True)['passed'])
    def test_old_executable_ignoring_flag_refused(self):self.assertFalse(q.native_only_evidence('game ran normally\n',True)['passed'])
    def test_no_report_or_no_arm_or_duplicate_report_refused(self):
        for text in (ARM,SUMMARY,ARM+ARM+SUMMARY,ARM+SUMMARY+SUMMARY):
            with self.subTest(text=text):self.assertFalse(q.native_only_evidence(text,True)['passed'])
    def test_each_nonzero_execution_or_failure_counter_refused(self):
        for key in ('interpreted_opcodes','completed_opcodes','runtime_invocations','unqualified_replay_activations','blocked_events','bridge_uncovered_attempts'):
            with self.subTest(key=key):self.assertFalse(q.native_only_evidence(ARM+SUMMARY.replace(key+'=0',key+'=1'),True)['passed'])
    def test_native_and_bypass_counts_are_not_decoder_counts(self):
        proof=q.native_only_evidence(ARM+SUMMARY,True);self.assertTrue(proof['passed']);self.assertEqual(proof['counters']['generic_accelerations_bypassed'],17)
    def test_bad_numeric_duplicate_missing_or_wrong_schema_refused(self):
        for modified in (SUMMARY.replace('strict=1','strict=0'),SUMMARY.replace('schema=1','schema=2'),SUMMARY.replace('blocked_events=0',''),SUMMARY.replace('blocked_events=0','blocked_events=0 blocked_events=0'),SUMMARY.replace('blocked_events=0','blocked_events=-1'),SUMMARY.replace('blocked_events=0','blocked_events=1e9999'),SUMMARY.replace('blocked_events=0','blocked_events=false'),SUMMARY.replace('blocked_events=0','blocked_events=00')):
            with self.subTest(modified=modified):self.assertFalse(q.native_only_evidence(ARM+modified,True)['passed'])
    def test_fatal_message_cannot_hide_behind_zero_summary(self):
        log=ARM+'[native-only] first-uncovered pc=008013 reason=bridge-prefetch\n'+SUMMARY
        self.assertFalse(q.native_only_evidence(log,True)['passed']);self.assertEqual(len(q.runtime_failure_lines(log)),1)
    def test_counter_overflow_fails_closed_without_parse_exception(self):
        for value in ('18446744073709551616','9'*5000):
            with self.subTest(length=len(value)):self.assertFalse(q.native_only_evidence(ARM+SUMMARY.replace('blocked_events=0','blocked_events='+value),True)['passed'])
    def test_policy_boolean_literal(self):
        for value in ('0','1'):q.validate_diagnostic_boolean('SNESRECOMP_NATIVE_ONLY',value)
        for value in ('true','1\n','-1','00'):
            with self.subTest(value=value),self.assertRaises(SystemExit):q.validate_diagnostic_boolean('SNESRECOMP_NATIVE_ONLY',value)
    def test_generated_backend_requires_actual_runtime_capability(self):
        logs='[native-blocks69] registered program=lagoon_reset69 profile=0 entries=39 source_sha256='+'a'*64+'\n[native-blocks69] bound generation=1 profile=0\n'
        strict=q.native_only_evidence(ARM+SUMMARY.replace('schema=1 strict=1','schema=1 strict=1 generated_native_opcodes=401435'),True)
        self.assertTrue(q.native_backend_evidence(logs,True,strict)['passed'])
        self.assertIsNone(q.native_backend_evidence(logs,False,strict))
        for text,proof in (('',strict),(logs,None),(logs,q.native_only_evidence(ARM+SUMMARY,True)),(logs.replace('bound generation=1 profile=0','bound generation=1 profile=1'),strict),(logs+logs,strict),(logs+'[native-blocks69] contract-refused reason=mapping\n',strict)):
            with self.subTest(text=text):self.assertFalse(q.native_backend_evidence(text,True,proof)['passed'])
    def test_backend_comparison_changes_only_proven_execution_policy(self):
        left={'exe_sha256':'a'*64,'program_manifest_sha256':'b'*64}
        right={**left,'native_only':{'passed':True},'native_backend':{'passed':True,'generated_native_opcodes':123}}
        old={'SNESRECOMP_NATIVE_ONLY':'0','LAGOON_NATIVE_BACKEND69':'0','LAGOON_NATIVE_BOOT62':'off'}
        new={**old,'SNESRECOMP_NATIVE_ONLY':'1','LAGOON_NATIVE_BACKEND69':'1'}
        self.assertTrue(compare.backend69_control(left,right,old,new)['approved'])
        for candidate,overrides in (({**right,'exe_sha256':'c'*64},new),({**right,'native_only':{'passed':False}},new),({**right,'native_backend':{'passed':True,'generated_native_opcodes':0}},new),(right,{**new,'LAGOON_NATIVE_BOOT62':'on'}),(right,{**new,'SNESRECOMP_NATIVE_ONLY':'0'})):
            self.assertFalse(compare.backend69_control(left,candidate,old,overrides)['approved'])
if __name__=='__main__':unittest.main()
