import bpy,math,json
from pathlib import Path
from mathutils import Vector
from PIL import Image,ImageDraw,ImageFont
OUT=Path(__file__).resolve().parent
FRAMES=OUT.parents[3]/'Saved/CodexModeling/gunner_armed_20261102/frames';FRAMES.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Gunner_Armed_Animated.blend'))
rig=bpy.data.objects['Armature'];scene=bpy.context.scene
scene.cycles.samples=8;scene.render.resolution_x=600;scene.render.resolution_y=700
scene.render.resolution_percentage=100;camera=scene.camera
def point_at(ob,target):ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()
rig.animation_data.action=bpy.data.actions['GUN_HoldIdle'];scene.frame_set(1)
camera.location=(-270,-340,166);point_at(camera,(-7,-20,126));camera.data.ortho_scale=95
scene.render.resolution_x=1000;scene.render.resolution_y=1000;scene.cycles.samples=24
scene.render.filepath=str(OUT/'Preview_GripCheck.png');bpy.ops.render.render(write_still=True)
camera.location=(285,-600,238);point_at(camera,(-2,-20,98));camera.data.ortho_scale=221
scene.render.resolution_x=600;scene.render.resolution_y=700;scene.cycles.samples=8
# 20 fps avoids the six identical barrels aliasing to a stationary rotor at 12 fps.
for action,count,label in [('GUN_Walk',30,'SILAHLI YURUME'),('GUN_FireLoop',20,'SUREKLI TARAMA'),('GUN_WalkFire',30,'YURURKEN TARAMA')]:
    rig.animation_data.action=bpy.data.actions[action];duration=1 if count==20 else 1.5;images=[]
    for i in range(count):
        f=1+i/count*duration*30;scene.frame_set(int(f),subframe=f-int(f))
        path=FRAMES/f'{action}_{i:03}.png';scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)
        image=Image.open(path).convert('RGB');draw=ImageDraw.Draw(image);font=ImageFont.truetype(r'C:\Windows\Fonts\bahnschrift.ttf',20)
        draw.text((20,20),label,font=font,fill=(190,215,73));images.append(image)
    palette=images[0].quantize(colors=128,method=Image.Quantize.MEDIANCUT)
    frames=[im.quantize(palette=palette,dither=Image.Dither.NONE) for im in images]
    frames[0].save(OUT/(action+'.gif'),save_all=True,append_images=frames[1:],duration=50,loop=0,disposal=2)
    print('ANIMATED_PREVIEW_READY',action,flush=True)
print('MOTION_PREVIEWS_COMPLETE',flush=True)
