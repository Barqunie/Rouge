"""Authored low-poly Gunner and separate minigun; bpy 4.2+, numpy, Pillow."""
import bpy,bmesh,math,random,json
import numpy as np
from pathlib import Path
from mathutils import Vector,Matrix
from PIL import Image
OUT=Path(__file__).resolve().parent
BASE=OUT.parent/'AlchemistMage'
random.seed(1307)
bpy.ops.wm.open_mainfile(filepath=str(BASE/'AlchemistMage_APose.blend'))
rig=bpy.data.objects['Armature'];rig.animation_data_clear()
for p in rig.pose.bones:p.matrix_basis=Matrix.Identity(4)
for ob in list(bpy.data.objects):
    if ob!=rig:bpy.data.objects.remove(ob,do_unlink=True)
bpy.context.view_layer.objects.active=rig;rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for b in list(rig.data.edit_bones):
    if b.name.startswith('coat_'):rig.data.edit_bones.remove(b)
for side,s in [('l',1),('r',-1)]:
    pts=[(s*.061,.066,1.77),(s*.092,.103,1.64),(s*.105,.119,1.54),(s*.13,.132,1.45)]
    for i in range(3):
        b=rig.data.edit_bones[f'hair_{side}_{i+1:02}'];b.head=Vector(pts[i])*100;b.tail=Vector(pts[i+1])*100
bpy.ops.object.mode_set(mode='OBJECT')
rig.data.name='SK_Gunner'
scene=bpy.context.scene;S=100.;PARTS=[]
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
bpy.context.preferences.filepaths.save_version=0
BONES={b.name:(b.head_local/S,b.tail_local/S,b.parent.name if b.parent else None,b.use_deform) for b in rig.data.bones}
MATDEF={
 'Skin':('#bc876d',.78,0),'SkinLight':('#d49c7e',.78,0),'Tattoo':('#765a4c',.85,0),
 'Cloth':('#51503b',.90,0),'ClothLight':('#5e5942',.9,0),'ClothDark':('#383a2d',.9,0),
 'Leather':('#322c29',.76,0),'LeatherEdge':('#65503e',.72,.05),
 'Metal':('#514941',.60,.55),'MetalLight':('#8c7860',.55,.55),
 'Hair':('#542936',.80,0),'HairLight':('#6b3543',.8,0),'HairDark':('#39212b',.84,0),
 'Wrap':('#b49e78',.92,0),'Toxic':('#b3d51c',.37,.1),'ToxicDark':('#687c25',.57,.1),
 'Shadow':('#171b19',.82,0),'White':('#e6d4b2',.66,0),'Iris':('#936935',.57,0),
 'Brass':('#ac8b4b',.57,.5)
}
MATS={}
def linear(v):return v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4
for name,(hexcolor,rough,metal) in MATDEF.items():
    rgb=[int(hexcolor[i:i+2],16)/255 for i in [1,3,5]];rgba=tuple(linear(c) for c in rgb)+(1,)
    mat=bpy.data.materials.new('M_Gunner_'+name);mat.use_nodes=True;mat.diffuse_color=rgba
    p=mat.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=rgba;p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    if name=='Toxic':p.inputs['Emission Color'].default_value=rgba;p.inputs['Emission Strength'].default_value=.3
    MATS[name]=mat
exec((OUT/'geometry.py').read_text())

def panel(name,points,mat,weight='pelvis',thick=.002):
    pts=[Vector(p) for p in points];normal=(pts[1]-pts[0]).cross(pts[2]-pts[0]).normalized()
    n=len(pts);verts=pts+[p-normal*thick for p in pts]
    faces=[tuple(range(n)),tuple(reversed(range(n,2*n)))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,verts,faces,mat,weight)

def band(name,a,b,r,mat,weight='pelvis',sides=10,depth=None):
    return tube(name,[(a,r,r if depth is None else depth),(b,r,r if depth is None else depth)],mat,weight,sides,1)

def hoop(name,c,axis,radius,thickness,mat,weight='pelvis',sides=12):
    c=Vector(c);axis=Vector(axis).normalized();u=axis.cross(Vector((0,0,1)))
    if u.length<.001:u=axis.cross(Vector((0,1,0)))
    u.normalize();v=axis.cross(u);vs=[];fs=[]
    for i in range(sides):
        a=i*math.tau/sides
        for j in range(4):
            b=j*math.tau/4;vs.append(c+(u*math.cos(a)+v*math.sin(a))*(radius+thickness*math.cos(b))+axis*(thickness*math.sin(b)))
    for i in range(sides):
        for j in range(4):fs.append((i*4+j,((i+1)%sides)*4+j,((i+1)%sides)*4+(j+1)%4,i*4+(j+1)%4))
    return mesh(name,vs,fs,mat,weight)

def hazard(name,c,size,mat,weight='pelvis',back=False):
    c=Vector(c);normal=Vector((0,1 if back else -1,0))
    outer=[Vector((-size*.5,0,-size*.34)),Vector((size*.5,0,-size*.34)),Vector((0,0,size*.52))]
    for i in range(3):
        a,b=outer[i],outer[(i+1)%3]
        panel(name+'_border',[c+a,c+b,c+b*.72,c+a*.72],mat,weight,.0005)
    for angle in [0,math.tau/3,math.tau*2/3]:
        pts=[]
        for r,a in [(size*.07,angle-.2),(size*.21,angle-.45),(size*.21,angle+.45),(size*.07,angle+.2)]:pts.append(c+Vector((math.sin(a)*r,0,math.cos(a)*r)))
        panel(name+'_trefoil',pts,mat,weight,.0004)

exec((OUT/'character.py').read_text())
exec((OUT/'head.py').read_text())
CHAR_PARTS=list(PARTS);PARTS=[]
exec((OUT/'minigun.py').read_text())
GUN_PARTS=list(PARTS);PARTS=[]
exec((OUT/'export_render.py').read_text())
