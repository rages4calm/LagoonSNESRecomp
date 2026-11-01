"""Reject ignored, partial, malformed or miscounted native coverage captures."""
import copy,importlib.util,json,tempfile,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('qualifier_coverage69',Path(__file__).resolve().parents[1]/'tools/qualify_native.py')
q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
ARM='[native-block-coverage] armed schema=1 processor=main65816 kind=generated-native-blocks pid=7\n'
END='[native-block-coverage] finalized schema=1 finalization=normal-exit generated_native_opcodes=3 hits_total=3 programs=1 nonzero_keys=1 errors=0 pid=7\n'
DOC=dict(schema=1,processor='main65816',kind='generated-native-blocks',pid=7,finalization='normal-exit',counter_consistent=True,errors=0,hits_total=3,generated_native_opcodes=3,program_count=1,nonzero_key_count=1,programs=[dict(name='kernel',profile=0,original_source_sha256='a'*64,compiled_key_count=1,hits_total=3,keys=[dict(pc24=0x8013,m=1,x=1,e=1,profile=0,hits=3)])])
class CaptureContract(unittest.TestCase):
    def proof(self,doc=None,log=ARM+END,strict=3,raw=None):
        with tempfile.TemporaryDirectory()as temp:
            file=Path(temp)/'capture.json';file.write_text(raw if raw is not None else json.dumps(DOC if doc is None else doc))
            return q.native_block_coverage_evidence(file,log,None if strict is None else {'counters':{'generated_native_opcodes':strict}})
    def test_positive_independent_total_and_program_rows(self):
        result=self.proof();self.assertTrue(result['passed']);self.assertEqual(result['hits_total'],3);self.assertEqual(result['programs'][0]['nonzero_keys'],1)
    def test_missing_or_ambiguous_ack_refused(self):
        for log in ('',ARM,END,ARM+ARM+END,ARM+END+END,ARM.replace('pid=7','pid=8')+END):
            self.assertFalse(self.proof(log=log)['passed'])
    def test_partial_or_inconsistent_header_refused(self):
        for key,value in [('finalization','partial'),('counter_consistent',False),('counter_consistent',1),('errors',1),('program_count',2),('nonzero_key_count',2),('schema',True),('hits_total',4),('generated_native_opcodes',4),('pid',8)]:
            doc=copy.deepcopy(DOC);doc[key]=value;self.assertFalse(self.proof(doc)['passed'])
    def test_strict_independent_counter_must_match(self):self.assertFalse(self.proof(strict=4)['passed'])
    def test_bad_native_key_and_counts_refused(self):
        for key,value in [('hits',True),('hits',0),('hits',4),('m',0),('profile',1),('pc24',0x1000000),('x',-1)]:
            doc=copy.deepcopy(DOC);doc['programs'][0]['keys'][0][key]=value;self.assertFalse(self.proof(doc)['passed'])
    def test_duplicate_key_refused_even_with_consistent_totals(self):
        doc=copy.deepcopy(DOC);p=doc['programs'][0];p['keys']*=2;p['compiled_key_count']=2;p['hits_total']=6
        doc.update(hits_total=6,generated_native_opcodes=6,nonzero_key_count=2)
        self.assertFalse(self.proof(doc,log=ARM+END.replace('=3','=6').replace('nonzero_keys=1','nonzero_keys=2'),strict=6)['passed'])
    def test_bad_json_types_duplicate_fields_nonfinite_refused(self):
        for raw in ('[]','null','{"schema":1,"schema":1}',json.dumps(DOC).replace('"hits": 3','"hits": 1e9999')):
            self.assertFalse(self.proof(raw=raw)['passed'])
    def test_no_requested_capture_is_not_a_claim(self):self.assertIsNone(q.native_block_coverage_evidence(None,'',None))
    def test_empty_reference_capture_is_observation_only(self):
        doc=copy.deepcopy(DOC);doc.update(programs=[],program_count=0,nonzero_key_count=0,hits_total=0,generated_native_opcodes=0)
        end=END.replace('=3','=0').replace('programs=1','programs=0').replace('nonzero_keys=1','nonzero_keys=0')
        self.assertTrue(self.proof(doc,log=ARM+end,strict=None)['passed'])
if __name__=='__main__':unittest.main()
