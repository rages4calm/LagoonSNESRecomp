"""ROM-free old/new observer state and redundant-clear proof from actual source.

Reference is public source commit953d4d2 (requires that local Git object). No ROM,
save or executable is embedded; source identity predicates are explicit fixtures.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

BASE='953d4d2d3ea1018685f4070a0c675124e44fb87d'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--compiler',default='clang')
    args=parser.parse_args()
    if args.out.exists():parser.error('Choose fresh scratch output')
    repo=Path(__file__).resolve().parents[1]
    utility=repo/'tools/test_audio_load_resync.py'
    spec=importlib.util.spec_from_file_location('extract',utility)
    extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
    files=['src/lagoon_wide_entities.c','src/lagoon_widescreen.c','src/lagoon_wide_spell3.c']
    current={name:(repo/name).read_text(encoding='utf8')for name in files}
    original={name:subprocess.check_output(['git','show',BASE+':'+name],cwd=repo).decode('utf8')for name in files}
    entities,wide,spell=(current[x]for x in files)
    old_entities,old_wide,old_spell=(original[x]for x in files)
    new_observer=extract.block(entities,'static void observe_vram_byte(uint32_t adr,uint8_t value) {')
    old_observer=extract.block(old_entities,'static void observe_vram_byte(uint32_t adr,uint8_t value) {')
    reference=[];optimized=[]
    for atlas,kind in [('s_magic_atlas','Magic'),('s_death_atlas','Death'),('s_atlas','Atlas')]:
        old=re.search(r'if\s*\(\s*'+atlas+r'\.generation!=RtlStateGeneration\(\)\)\s*LagoonWide'+kind+r'_Reset\(&'+atlas+r'\);',old_observer)[0]
        new=extract.block(new_observer,'if('+atlas+'.generation!=generation) {')
        reference.append(old);optimized.append(new)
        old_observer=old_observer.replace(old,'GENERATION_'+kind+';')
        new_observer=new_observer.replace(new,'GENERATION_'+kind+';')
    new_observer=new_observer.replace('  uint64_t generation=RtlStateGeneration();\n','')
    normalize=lambda text:re.sub(r'\s+','',extract.mask_literals(text))
    assert normalize(old_observer)==normalize(new_observer),'An unrelated observer write/invalidation changed'
    old_callback=extract.block(old_wide,'void LagoonWide_OnVramWrite(unsigned byte_address) {')
    new_callback=extract.block(wide,'void LagoonWide_OnVramWrite(unsigned byte_address) {')
    old=re.search(r'if\(s_background.generation!=RtlStateGeneration\(\)\)LagoonWideBackground_Reset\(&s_background\);',old_callback)[0]
    new=extract.block(new_callback,'if(s_background.generation!=generation) {')
    reference.append(old);optimized.append(new)
    assert normalize(old_callback.replace(old,'GENERATION_BG;'))==normalize(new_callback.replace(new,'GENERATION_BG;').replace('  uint64_t generation=RtlStateGeneration();\n',''))
    signatures=[
        'void LagoonWideMagic_Reset(LagoonWideMagicAtlas *a)',
        'int LagoonWideMagic_Begin(LagoonWideMagicAtlas *a,uint64_t generation,unsigned room,',
        'void LagoonWideMagic_Write(LagoonWideMagicAtlas *a,unsigned address,uint8_t value)',
        'void LagoonWideDeath_Reset(LagoonWideDeathAtlas *a)',
        'int LagoonWideDeath_Begin(LagoonWideDeathAtlas *a,uint64_t generation,',
        'void LagoonWideDeath_Write(LagoonWideDeathAtlas *a,unsigned address,uint8_t value)',
        'void LagoonWideAtlas_Reset(LagoonWideAtlas *a)',
        'int LagoonWideAtlas_Begin(LagoonWideAtlas *a, uint64_t gen,',
        'void LagoonWideAtlas_Write(LagoonWideAtlas *a, unsigned adr)',
        'void LagoonWideBackground_Reset(LagoonWideBackground *b)',
        'int LagoonWideBackground_Begin(LagoonWideBackground *b,uint64_t generation,',
        'void LagoonWideBackground_Write(LagoonWideBackground *b,unsigned address,uint8_t value)']
    routines=[]
    for name in signatures:
        body=extract.block(entities,name)
        assert body==extract.block(old_entities,name),'Shared Begin/Write/Reset contract changed'
        routines.append(body)
    tables=entities[entities.index('typedef struct MagicSource {'):entities.index('int LagoonWideMagic_Source(')]
    spell_state=spell[spell.index('enum { kGraphicsBytes='):spell.index('static unsigned word(')]
    spell_state=spell_state.replace('static struct {','typedef struct {').replace('} s_spell;','} SpellState;')
    helper_names=['static uint64_t hash(','void LagoonWideSpell3_Reset(void)','static void withdraw_maps(void)','static int graphics_address(unsigned a)','static int owned(unsigned a)','void LagoonWideSpell3_Dma(']
    for name in helper_names:
        assert extract.block(spell,name)==extract.block(old_spell,name),'Shared Spell3 helper contract changed'
    helpers='\n'.join(extract.block(spell,name)for name in helper_names)
    reference_spell=extract.block(old_spell,'void LagoonWideSpell3_Write(unsigned a,uint8_t value)').replace('LagoonWideSpell3_Write','reference_spell_write',1)
    optimized_spell=extract.block(spell,'void LagoonWideSpell3_Write(unsigned a,uint8_t value)').replace('LagoonWideSpell3_Write','optimized_spell_write',1)
    args.out.mkdir(parents=True)
    header=args.out/'observer_under_test.h'
    header.write_text(tables+'\n'+ '\n'.join(routines)+'\n'+helpers+'\n'+reference_spell+'\n'+optimized_spell+'\nstatic void reference_generation(void) {\n'+'\n'.join(reference)+'\n}\nstatic void optimized_generation(void) {\n uint64_t generation=RtlStateGeneration();\n'+'\n'.join(optimized)+'\n}\n',encoding='utf8')
    (args.out/'spell_state_under_test.h').write_text(spell_state,encoding='utf8')
    harness=repo/'tests/lagoon_observer_reset_test.c';exe=args.out/'observer-reset-test.exe'
    subprocess.run([args.compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(args.out.resolve()),'-I'+str(repo/'src'),str(harness),'-o',str(exe)],check=True)
    run=subprocess.run([str(exe.resolve())],capture_output=True,text=True);print(run.stdout,end='');print(run.stderr,end='')
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    result=dict(passed=run.returncode==0,returncode=run.returncode,reference_commit=BASE,stdout=run.stdout,stderr=run.stderr,sources={name:sha(repo/name)for name in files},reference_source_sha256={name:hashlib.sha256(original[name].encode()).hexdigest()for name in files},harness_sha256=sha(harness),driver_sha256=sha(Path(__file__)),extracted_sha256=sha(header),all_other_observer_write_and_invalidation_code_identical=True,actual_Begin_Write_Reset_bodies_unchanged=True,scope='Actual old/current generation clauses, shared actual state machines, old/new complete Spell3_Write and actual Spell3_Dma. Explicit source predicates are fixture booleans; not ROM/source-hash or guest/APU validation. Generation acknowledgment is the sole permitted empty-atlas difference; no ownership granted. Spell bytes/state exact for reachable acquisition/withdrawal/reset paths. Cleared bytes counted, not a hardware benchmark.')
    (args.out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
    if not result['passed']:raise SystemExit('Actual observer reset differential failed')


if __name__=='__main__':main()
