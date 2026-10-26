"""Render real skinned animation frames and small looping GIFs for review."""
import bpy,json,math
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
OUT=Path(__file__).resolve().parent;QA=OUT.parents[2]/'Saved/CodexModeling/mage_20261026'
framesdir=QA/'AnimationPreviewFrames';framesdir.mkdir(exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(OUT/'AlchemistMage_Animated.blend'))
scene=bpy.context.scene;rig=bpy.data.objects['Armature']
scene.render.resolution_x=440;scene.render.resolution_y=560;scene.cycles.samples=12
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',21)
small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',14)
info=json.loads((OUT/'animation_report.json').read_text())['clips']
stills=[]
for label,title in [('Walk','YURUME'),('Run','KOSMA'),('Cast','BUYU FIRLATMA')]:
    name='AM_'+label;rig.animation_data.action=bpy.data.actions[name]
    duration=info[name]['duration_seconds'];n=max(16,round(duration*14))
    frames=[]
    for i in range(n):
        frame=1+(info[name]['frames']-1)*i/n
        scene.frame_set(int(frame),subframe=frame-int(frame))
        path=framesdir/f'{name}_{i:03}.png';scene.render.filepath=str(path)
        bpy.ops.render.render(write_still=True)
        im=Image.open(path).convert('RGB');draw=ImageDraw.Draw(im)
        draw.rectangle((0,0,440,55),fill=(18,23,27));draw.text((20,10),title,font=font,fill=(207,232,169))
        draw.text((20,533),f'ALCHEMIST MAGE  /  {duration:.2f} s  /  UE 5.8',font=small,fill=(220,224,218))
        frames.append(im)
    delays=[round(duration*1000/n)]*n
    if label=='Cast':frames.append(frames[-1].copy());delays.append(450)
    frames[0].save(OUT/f'Preview_{label}.gif',save_all=True,append_images=frames[1:],duration=delays,loop=0,optimize=False,disposal=2)
    stills.append(frames[round(n*(.60/duration if label=='Cast' else .25))])
    print('PREVIEW_COMPLETE',label,n,flush=True)
sheet=Image.new('RGB',(1320,560),(18,23,27))
for i,im in enumerate(stills):sheet.paste(im,(i*440,0))
sheet.save(OUT/'Preview_Animations.png')
print('PREVIEWS_COMPLETE',flush=True)
