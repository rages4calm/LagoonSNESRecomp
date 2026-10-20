"""Optional, read-only Jev screen triage for assisted coverage captures.

Requires TYPESAFE_API_KEY and Pillow. This tool never controls the game or
accepts native/render parity; original memory, source and pixel checks do that.
API contract: https://docs.typesafe.ai/images and /api/system-one.
"""
import argparse,base64,hashlib,io,json,math,os,time,urllib.request,uuid
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--images',type=Path,nargs='+',required=True)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--model',default='jev-latest')
    p.add_argument('--view',choices=('native','expanded'),default='native',
                   help='Describe the actual supplied capture; expanded includes widescreen margins')
    p.add_argument('--dry-run',action='store_true')
    a=p.parse_args()
    if a.out.exists():p.error('--out must be a fresh file')
    if not 1<=len(a.images)<=12:p.error('Use one bounded batch of at most12 images')
    from PIL import Image
    assets={};scenes={};questions={};bindings=[]
    for i,path in enumerate(a.images):
        raw=path.read_bytes();ref=str(uuid.uuid4());key=f'scene{i:02d}'
        with Image.open(io.BytesIO(raw)) as original:
            size=list(original.size);scale=max(1,math.ceil(280/min(size)))
            # The image API requires both dimensions >=280. Integer nearest
            # duplication preserves the source pixels; this is analysis only.
            image=original.convert('RGB')
            if scale>1:image=image.resize((size[0]*scale,size[1]*scale),Image.Resampling.NEAREST)
            if max(image.size)>1568:p.error('Capture exceeds the documented image dimension limit')
            b=io.BytesIO();image.save(b,format='PNG');sent_size=list(image.size)
        assets[ref]={'type':'image','format':'png','data_base64':base64.b64encode(b.getvalue()).decode()}
        scenes[key]={'capture':ref,'scope':('Original native game view; excludes expanded widescreen margins'
            if a.view=='native' else 'Complete expanded game presentation, including widescreen scenery and actor margins')}
        bindings.append({'id':key,'path':str(path.resolve()),'sha256':hashlib.sha256(raw).hexdigest(),'size':size,'analysis_size':sent_size,'integer_nearest_scale':scale})
        questions[key]={'type':'choice','instructions':f'Classify only the observed screen in `{key}.capture`. Recommend the next bounded test action from its visible state.',
            'criteria':{'continue_route':'A readable ordinary gameplay scene, possibly with HUD or actors.','dismiss_dialogue':'A readable dialogue/message that should be advanced.','close_menu':'An inventory, equipment or other selectable menu.','wait_transition':'A blank, fading, loading, title or cinematic screen.','inspect_frame':'Random visual noise or a scene too broken or ambiguous to continue automatically.'}}
    if a.dry_run:
        print(json.dumps({'passed':True,'images':len(bindings),'writes':0,'requests':0,'bindings':bindings},indent=2));return
    key=os.environ.get('TYPESAFE_API_KEY')
    if not key:p.error('TYPESAFE_API_KEY is not configured')
    request={'model':a.model,'state':scenes,'assets':assets,'questions':questions}
    started=time.monotonic()
    req=urllib.request.Request('https://api.typesafe.ai/v1/systemone',data=json.dumps(request).encode(),headers={'Authorization':'Bearer '+key,'Content-Type':'application/json','TypeSafe-Danger-Zone-Images':'true'})
    try:
        with urllib.request.urlopen(req,timeout=60) as response:result=json.load(response)
    except urllib.error.HTTPError as error:
        detail=error.read(2048).decode(errors='replace')
        raise SystemExit(f'TypeSafe HTTP {error.code}: {detail}') from None
    if set(result.get('answers',{}))!=set(questions):raise ValueError('Incomplete Jev answers')
    review=[name for name,answer in result['answers'].items() if answer.get('confidence',0)<0.65 or answer.get('choice')=='inspect_frame']
    out={'schema':'Lagoon optional Jev coverage triage v1','view':a.view,'model':result.get('model'), 'elapsed_seconds':time.monotonic()-started,'usage':result.get('usage'),'answers':result['answers'],'bindings':bindings,'review_queue':review,'game_actions_executed':0,'acceptance_authority':False}
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(out,indent=2)+'\n',encoding='utf8')
    print(json.dumps({'model':out['model'],'images':len(bindings),'elapsed_seconds':out['elapsed_seconds'],'usage':out['usage'],'answers':out['answers']},indent=2))

if __name__=='__main__':main()
