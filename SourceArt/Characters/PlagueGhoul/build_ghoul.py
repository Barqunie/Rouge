"""Reproducible low-poly ghoul, A-pose bind skeleton, FBX/GLB and studio renders.
Run with Blender 4.2+ Python or a Python environment containing bpy and Pillow.
Geometry is authored in metres and exported in centimetres for Unreal.
"""
import bpy
import bmesh
import math
import random
import json
from pathlib import Path
from mathutils import Vector
from PIL import Image

OUT = Path(__file__).resolve().parent
OUT.mkdir(parents=True, exist_ok=True)
random.seed(37)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 0.01
PARTS = []
BONES = {}
S = 100.0
# Keep the A-pose direction, but bring the arms/oversized hands closer to human proportions.
ARM_LENGTH_SCALE = 0.64
HAND_SCALE = 0.62
bpy.context.preferences.filepaths.save_version = 0

def bone(name, head, tail, parent=None, deform=True):
    BONES[name] = (Vector(head), Vector(tail), parent, deform)

bone('root', (0,0,0), (0,0,.16))
bone('pelvis', (0,0,1.29), (0,0,1.43), 'root')
spine = [(0,0,1.43), (0,.015,1.56), (0,.03,1.68), (0,.045,1.81), (0,.045,1.94), (0,.02,2.055)]
for i in range(5): bone(f'spine_{i+1:02}',spine[i],spine[i+1], 'pelvis' if i==0 else f'spine_{i:02}')
bone('neck_01',spine[-1],(0,-.06,2.20),'spine_05')
bone('neck_02',(0,-.06,2.20),(0,-.13,2.34),'neck_01')
bone('head',(0,-.13,2.34),(0,-.13,2.77),'neck_02')
bone('jaw',(0,-.14,2.35),(0,-.39,2.08),'head')

PALETTE = {
 'skin': (169,173,105), 'bone':(185,174,143), 'shirt':(139,59,46),
 'trousers':(76,61,47), 'cavity':(27,28,17), 'toxic':(185,220,58),
 'nail':(130,143,75), 'crack':(65,61,42)
}
palette = Image.new('RGBA',(256,256))
pix=palette.load()
for row,(label,rgb) in enumerate(PALETTE.items()):
    for col in range(8):
        f=[.76,.84,.91,.97,1.0,1.04,1.09,1.14][col]
        color=tuple(min(255,round(x*f)) for x in rgb)+(255,)
        for y in range(row*32,(row+1)*32):
            for x in range(col*32,(col+1)*32): pix[x,y]=color
palette_path=OUT/'T_PlagueGhoul_Palette.png'
palette.save(palette_path)
tex=bpy.data.images.load(str(palette_path))
tex.pack()
materials=[]
for glow in [False,True]:
    mat=bpy.data.materials.new('M_PlagueGhoul_Toxic' if glow else 'M_PlagueGhoul_Body')
    mat.use_nodes=True
    mat.diffuse_color=(.55,.62,.18,1) if glow else (.48,.37,.23,1)
    node=mat.node_tree.nodes.get('Principled BSDF')
    node.inputs['Roughness'].default_value=.85 if not glow else .5
    node.inputs['Specular IOR Level'].default_value=.22
    t=mat.node_tree.nodes.new('ShaderNodeTexImage');t.image=tex;t.interpolation='Closest'
    mat.node_tree.links.new(t.outputs['Color'],node.inputs['Base Color'])
    if glow:
        mat.node_tree.links.new(t.outputs['Color'],node.inputs['Emission Color'])
        node.inputs['Emission Strength'].default_value=.23
    materials.append(mat)

def paint(obj, label, glowing=False, variation=True):
    obj.data.materials.clear();obj.data.materials.append(materials[int(glowing)])
    for old_uv in list(obj.data.uv_layers): obj.data.uv_layers.remove(old_uv)
    uv=obj.data.uv_layers.new(name='UVMap')
    row=list(PALETTE).index(label)
    for p in obj.data.polygons:
        col=random.choices(range(8),weights=[1,2,4,8,10,8,4,1])[0] if variation else 4
        for j,li in enumerate(p.loop_indices):
            a=j*math.tau/len(p.loop_indices)
            uv.data[li].uv=((col+.5)/8+math.cos(a)*.015,1-(row+.5)/8+math.sin(a)*.015)
        p.use_smooth=False

def bind(obj, weights):
    for v in obj.data.vertices:
        w=weights(v.co/S) if callable(weights) else {weights:1}
        total=sum(w.values())
        for name,val in w.items():
            if val>0:
                group=obj.vertex_groups.get(name) or obj.vertex_groups.new(name=name)
                group.add([v.index],val/total,'REPLACE')

def mesh(name, verts, faces, color, weights='head', glowing=False, triangulate=True):
    data=bpy.data.meshes.new(name)
    data.from_pydata([tuple(Vector(v)*S) for v in verts],[],faces);data.update()
    bm=bmesh.new();bm.from_mesh(data)
    if triangulate: bmesh.ops.triangulate(bm,faces=list(bm.faces),quad_method='BEAUTY',ngon_method='BEAUTY')
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(data);bm.free()
    obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj)
    paint(obj,color,glowing);bind(obj,weights);PARTS.append(obj)
    return obj

def ellipsoid(name, center, radii, color, weights='head', subdivisions=1, glowing=False):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdivisions,radius=1)
    obj=bpy.context.object;obj.name=name
    for v in obj.data.vertices: v.co=Vector(tuple((center[i]+v.co[i]*radii[i])*S for i in range(3)))
    paint(obj,color,glowing);bind(obj,weights);PARTS.append(obj)
    return obj

def loft(name, rings, color, weights, sides=10, jagged=None, cap=True):
    # rings: centre, cross-section width, depth. Tangent follows ring centres.
    verts=[];faces=[]
    for i,(point,rw,rd) in enumerate(rings):
        c=Vector(point)
        tangent=Vector(rings[min(i+1,len(rings)-1)][0])-Vector(rings[max(i-1,0)][0])
        tangent.normalize()
        front=Vector((0,-1,0))
        if abs(tangent.dot(front))>.9: front=Vector((0,0,1))
        u=tangent.cross(front).normalized();v=u.cross(tangent).normalized()
        for j in range(sides):
            theta=j*math.tau/sides
            p=c+u*(math.cos(theta)*rw)+v*(math.sin(theta)*rd)
            if jagged and i==len(rings)-1: p+=tangent*jagged[j%len(jagged)]
            verts.append(p)
        if i:
            for j in range(sides):
                a=(i-1)*sides+j;b=(i-1)*sides+(j+1)%sides
                faces.append((a,b,i*sides+(j+1)%sides,i*sides+j))
    if cap:
        faces.append(tuple(reversed(range(sides))))
        faces.append(tuple((len(rings)-1)*sides+j for j in range(sides)))
    return mesh(name,verts,faces,color,weights)

def zloft(name, rings, color, weights, sides=12, offsets=None):
    verts=[];faces=[]
    for i,(z,rx,ry,cy) in enumerate(rings):
        for j in range(sides):
            a=math.tau*j/sides
            dz=offsets(i,j,a) if offsets else 0
            verts.append((math.cos(a)*rx,cy+math.sin(a)*ry,z+dz))
        if i:
            for j in range(sides): faces.append(((i-1)*sides+j,(i-1)*sides+(j+1)%sides,i*sides+(j+1)%sides,i*sides+j))
    faces += [tuple(reversed(range(sides))),tuple((len(rings)-1)*sides+j for j in range(sides))]
    return mesh(name,verts,faces,color,weights)

def chain_weights(chain, blend=.08):
    def fn(p):
        best=None
        for name in chain:
            a,b,_,_=BONES[name];d=b-a
            t=max(0,min(1,(p-a).dot(d)/d.length_squared))
            dist=(p-a-d*t).length
            if best is None or dist<best[0]: best=(dist,name,t)
        _,name,t=best;i=chain.index(name);length=(BONES[name][1]-BONES[name][0]).length
        if i and t*length<blend:
            w=.5+.5*t*length/blend;return {name:w,chain[i-1]:1-w}
        if i<len(chain)-1 and (1-t)*length<blend:
            w=.5+.5*(1-t)*length/blend;return {name:w,chain[i+1]:1-w}
        return {name:1}
    return fn

body_weights=chain_weights(['pelvis']+[f'spine_{i:02}' for i in range(1,6)]+['neck_01','neck_02'],.065)
zloft('Body_under_tunic',[(1.24,.20,.14,0),(1.43,.19,.12,0),(1.64,.23,.15,.025),(1.85,.32,.18,.035),(2.02,.30,.18,.04),(2.12,.14,.105,-.01)],'skin',body_weights)
loft('Sinewy_neck',[((0,.02,1.94),.17,.12),((0,-.045,2.10),.12,.10),((0,-.11,2.24),.09,.09),((0,-.14,2.37),.11,.10)],'bone',body_weights,sides=9)
for s in [-1,1]:
    loft('Neck_tendon',[((s*.16,-.105,1.96),.027,.025),((s*.085,-.16,2.17),.027,.02),((s*.12,-.21,2.35),.032,.025)],'skin',body_weights,6)

def shirt_offsets(i,j,a):
    if i==0: return [-.06,.02,-.035,.015,-.075,.03,-.04,.035,-.055,.015,-.09,.035][j]
    if i==4: return -.23*max(0,-math.sin(a))**5
    return 0
zloft('Ragged_red_tunic',[(1.34,.235,.155,-.005),(1.46,.212,.147,0),(1.68,.27,.174,.03),(1.91,.355,.213,.04),(2.09,.275,.173,.04)],'shirt',body_weights,12,shirt_offsets)
for s in [-1,1]:
    mesh('Torn_lapel',[(s*.03,-.175,1.855),(s*.16,-.155,2.12),(s*.28,-.12,2.08),(s*.19,-.205,1.95),(s*.13,-.205,1.79)],[(0,1,3),(1,2,3),(0,3,4)],'shirt',body_weights)
zloft('Trouser_waist',[(1.12,.27,.17,.01),(1.30,.26,.174,0),(1.40,.22,.15,0)],'trousers','pelvis')

# Symmetric A-pose limbs, separate three-segment fingers and toe pivots.
for s,side in [(1,'l'),(-1,'r')]:
    shoulder=Vector((s*.34,.02,2.01))
    elbow=shoulder+Vector((s*.44,-.035,-.44))*ARM_LENGTH_SCALE
    wrist=elbow+Vector((s*.41,-.005,-.41))*ARM_LENGTH_SCALE
    hand_end=wrist+Vector((s*.18,-.005,-.18))
    axis=(hand_end-wrist).normalized();across=Vector((s*.707,0,.707))
    bone('clavicle_'+side,(s*.035,.02,2.04),shoulder,'spine_05')
    bone('upperarm_'+side,shoulder,elbow,'clavicle_'+side)
    bone('lowerarm_'+side,elbow,wrist,'upperarm_'+side)
    bone('hand_'+side,wrist,hand_end,'lowerarm_'+side)
    for base,a,b in [('upperarm',shoulder,elbow),('lowerarm',elbow,wrist)]:
        for n,t in [(1,.35),(2,.70)]: bone(f'{base}_twist_{n:02}_{side}',a.lerp(b,t),a.lerp(b,min(1,t+.17)),base+'_'+side)
    arm_w=chain_weights(['clavicle_'+side,'upperarm_'+side,'lowerarm_'+side,'hand_'+side],.085)
    p=lambda a,b,t: tuple(a.lerp(b,t))
    loft('Arm_'+side,[(shoulder,.119,.107),(p(shoulder,elbow,.22),.112,.099),(p(shoulder,elbow,.55),.088,.08),(p(shoulder,elbow,.91),.062,.061),(elbow,.065,.067),(p(elbow,wrist,.14),.079,.07),(p(elbow,wrist,.38),.076,.056),(p(elbow,wrist,.73),.045,.036),(wrist,.047*HAND_SCALE,.039*HAND_SCALE)],'skin',arm_w,8)
    loft('Ragged_sleeve_'+side,[(shoulder-axis*.06,.15,.14),(p(shoulder,elbow,.22),.153,.129),(p(shoulder,elbow,.49),.134,.114)],'shirt',arm_w,9,[d*ARM_LENGTH_SCALE for d in [.065,-.01,.10,-.02,.08,-.025,.13,.005,.09]])
    loft('Forearm_ridge_'+side,[(elbow+Vector((0,-.057,0)),.025,.015),(p(elbow,wrist,.45),.03,.018),(wrist,.017,.013)],'bone',arm_w,5)
    first_hand_part=len(PARTS)
    before_fingers=set(BONES)
    palm_mid=wrist.lerp(hand_end,.58)
    loft('Palm_'+side,[(wrist,.05,.039),(palm_mid,.09,.045),(hand_end,.107,.038)],'skin',arm_w,8)
    for idx,(finger,offset,length) in enumerate([('index',-.072,.265),('middle',-.019,.31),('ring',.036,.29),('pinky',.085,.225)]):
        start=hand_end+across*offset
        spread=across*((idx-1.3)*.017)
        a=start+axis*(length*.39)+spread+Vector((0,-.008,0))
        b=start+axis*(length*.76)+spread*1.4+Vector((0,-.026,0))
        tip=start+axis*(length*.98)+spread*1.7+Vector((0,-.075,0))
        meta=f'{finger}_metacarpal_{side}';bone(meta,wrist+across*offset*.6,start,'hand_'+side)
        chain=[]
        for k,(pa,pb) in enumerate([(start,a),(a,b),(b,tip)]):
            bn=f'{finger}_{k+1:02}_{side}';bone(bn,pa,pb,meta if k==0 else chain[-1]);chain.append(bn)
        fw=chain_weights(chain,.022)
        loft(f'{finger}_{side}',[(start,.027,.029),(start.lerp(a,.45),.025,.025),(a,.027,.027),(a.lerp(b,.5),.021,.02),(b,.023,.024),(tip,.011,.012)],'skin',fw,6)
        loft(f'Claw_{finger}_{side}',[(b.lerp(tip,.65)+Vector((0,-.012,.002)),.016,.007),(tip+axis*.035+Vector((0,-.026,0)),.002,.0015)],'nail',chain[-1],5)
    start=wrist.lerp(hand_end,.35)-across*.061
    a=start-across*.079+axis*.045
    b=a-across*.025+axis*.083+Vector((0,-.018,0))
    tip=b+axis*.065+across*.022+Vector((0,-.045,0))
    thumb=[]
    for k,(pa,pb) in enumerate([(start,a),(a,b),(b,tip)]):
        bn=f'thumb_{k+1:02}_{side}';bone(bn,pa,pb,'hand_'+side if k==0 else thumb[-1]);thumb.append(bn)
    loft('Thumb_'+side,[(start,.037,.035),(a,.031,.029),(b,.026,.023),(tip,.012,.01)],'skin',chain_weights(thumb,.022),6)
    loft('Thumb_claw_'+side,[(b.lerp(tip,.6),.018,.009),(tip+axis*.025+Vector((0,-.025,0)),.002,.002)],'nail',thumb[-1],5)
    # Scale palm, fingers, claws and their bind bones together about the wrist.
    # This preserves the authored weights and avoids an oversized hand on a short arm.
    for obj in PARTS[first_hand_part:]:
        for vertex in obj.data.vertices:
            vertex.co=wrist*S+(vertex.co-wrist*S)*HAND_SCALE
        obj.data.update()
    for name in (set(BONES)-before_fingers)|{'hand_'+side}:
        a,b,parent,deform=BONES[name]
        BONES[name]=(wrist+(a-wrist)*HAND_SCALE,wrist+(b-wrist)*HAND_SCALE,parent,deform)
    hip=Vector((s*.145,0,1.29));knee=Vector((s*.215,-.045,.695));ankle=Vector((s*.25,.005,.165));ball=Vector((s*.25,-.24,.075))
    bone('thigh_'+side,hip,knee,'pelvis');bone('calf_'+side,knee,ankle,'thigh_'+side)
    bone('foot_'+side,ankle,ball,'calf_'+side);bone('ball_'+side,ball,(s*.25,-.40,.065),'foot_'+side)
    for base,a,b in [('thigh',hip,knee),('calf',knee,ankle)]:
        for n,t in [(1,.35),(2,.7)]: bone(f'{base}_twist_{n:02}_{side}',a.lerp(b,t),a.lerp(b,min(1,t+.18)),base+'_'+side)
    leg_w=chain_weights(['thigh_'+side,'calf_'+side,'foot_'+side],.08)
    loft('Leg_'+side,[(hip,.13,.125),(hip.lerp(knee,.3),.12,.11),(hip.lerp(knee,.73),.087,.079),(hip.lerp(knee,.91),.074,.073),(knee,.083,.084),(knee.lerp(ankle,.13),.081,.078),(knee.lerp(ankle,.33),.094,.089),(knee.lerp(ankle,.68),.057,.053),(ankle,.047,.048)],'skin',leg_w,8)
    ellipsoid('Angular_knee_'+side,knee+Vector((0,-.065,0)),(.06,.033,.075),'bone',leg_w,1)
    loft('Shin_crest_'+side,[(knee+Vector((0,-.065,-.02)),.022,.015),(knee.lerp(ankle,.6)+Vector((0,-.047,0)),.02,.013),(ankle+Vector((0,-.04,.01)),.015,.01)],'bone',leg_w,5)
    loft('Tattered_trouser_leg_'+side,[(hip+Vector((0,0,.055)),.149,.158),(hip.lerp(knee,.25),.163,.146),(hip.lerp(knee,.48),.142,.12)],'trousers',leg_w,10,[.09,-.025,.075,-.02,.105,0,.06,-.02,.12,-.015])
    for j in range(3):
        x=s*(.11+j*.068);z=1.12+random.uniform(-.015,.03);y=-.14-j*.004
        mesh('Torn_cloth_strip_'+side+str(j),[(x-s*.05,y,z+.15),(x+s*.05,y,z+.10),(x+s*.04,y-.014,z-.06),(x-s*.012,y-.025,z-.18)],[(0,1,2),(0,2,3)],'trousers',leg_w)
    foot_w=chain_weights(['foot_'+side,'ball_'+side],.05)
    loft('Broad_foot_'+side,[((s*.25,.07,.10),.061,.074),((s*.25,-.005,.12),.074,.105),((s*.25,-.14,.085),.105,.064),((s*.25,-.25,.065),.126,.046),((s*.25,-.29,.058),.12,.037)],'skin',foot_w,8)
    for i in range(5):
        xx=s*(.25-.09+i*.047);end=-.405+i*.018;radius=.027-i*.0025
        loft(f'Toe_{i}_{side}',[((xx,-.25,.06),radius,radius*.9),((xx,-.32,.045),radius*.95,radius*.76),((xx,end,.034),radius*.82,radius*.64)],'skin','ball_'+side,6)
        ellipsoid(f'Toenail_{i}_{side}',(xx,end+.017,.052),(radius*.69,.024,.008),'nail','ball_'+side,1)

# Inflamed, faceted toxic growths on the shoulders and upper back.
for i,(x,y,z,r) in enumerate([(-.30,.12,2.11,.065),(-.19,.19,2.18,.073),(-.06,.22,2.22,.06),(.07,.23,2.23,.057),(.21,.18,2.19,.075),(.32,.10,2.11,.06),(-.28,.24,1.96,.052),(.26,.26,1.99,.048),(0,.265,2.05,.07)]):
    ellipsoid('Toxic_growth_'+str(i),(x,y,z-.065),(r,r*.83,r*.87),'toxic',body_weights,2)

# Skull. Actual recessed eye sockets are cut into the faceted cranium.
skull=zloft('Cranium',[(2.30,.165,.126,-.14),(2.40,.235,.177,-.145),(2.55,.257,.205,-.135),(2.69,.212,.168,-.12),(2.77,.105,.105,-.12),(2.80,.025,.03,-.115)],'bone','head',12)
for s in [-1,1]:
    bpy.ops.mesh.primitive_uv_sphere_add(segments=10,ring_count=6,radius=1)
    cutter=bpy.context.object;cutter.name='temporary_eye_cutter'
    cutter.location=(s*.133*S,-.315*S,2.424*S);cutter.scale=(.109*S,.139*S,.105*S)
    bpy.context.view_layer.objects.active=skull
    mod=skull.modifiers.new('Recessed_eye_socket','BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cutter
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter,do_unlink=True)
    ellipsoid('Dark_orbit',(s*.132,-.299,2.421),(.097,.061,.091),'cavity','head',2)
    ellipsoid('Toxic_eye',(s*.133,-.358,2.421),(.057,.049,.064),'toxic','head',2,True)
    # Inward-sloping brows, jutting cheekbones and a narrow upper jaw.
    verts=[(s*.035,-.365,2.482),(s*.238,-.323,2.54),(s*.247,-.298,2.488),(s*.061,-.407,2.452),(s*.053,-.321,2.522),(s*.221,-.272,2.577)]
    mesh('Heavy_brow',verts,[(0,1,2,3),(0,4,5,1),(1,5,2),(3,2,5,4),(0,3,4)],'bone')
    mesh('Cheekbone',[(s*.235,-.265,2.427),(s*.227,-.346,2.365),(s*.16,-.414,2.326),(s*.103,-.39,2.298),(s*.175,-.268,2.309),(s*.22,-.212,2.365)],[(0,1,2),(0,2,4,5),(2,3,4),(0,5,1),(1,5,4,3,2)],'bone')
    loft('Mandible_side', [((s*.183,-.16,2.365),.029,.033),((s*.173,-.215,2.22),.034,.028),((s*.12,-.35,2.095),.033,.032)],'bone','jaw',6)
loft('Jaw_chin',[((-.12,-.35,2.095),.031,.029),((-.068,-.40,2.075),.036,.03),((0,-.417,2.073),.033,.034),((.068,-.40,2.075),.036,.03),((.12,-.35,2.095),.031,.029)],'bone','jaw',6)
ellipsoid('Mouth_interior',(0,-.286,2.205),(.133,.085,.117),'cavity','head',2)
loft('Upper_maxilla',[((-.122,-.355,2.306),.031,.032),((-.065,-.397,2.299),.034,.031),((0,-.412,2.302),.031,.029),((.065,-.397,2.299),.034,.031),((.122,-.355,2.306),.031,.032)],'bone','head',6)
mesh('Nose_bridge',[(-.033,-.366,2.453),(.033,-.366,2.453),(-.059,-.414,2.326),(.059,-.414,2.326),(0,-.465,2.335),(0,-.369,2.39)],[(0,1,4),(0,4,2),(1,3,4),(0,2,5),(1,5,3),(2,4,3,5)],'bone')
for s in [-1,1]:
    mesh('Nasal_opening',[(s*.012,-.453,2.348),(s*.045,-.429,2.331),(s*.009,-.454,2.326)],[(0,1,2)],'cavity')
for i,x in enumerate([-.094,-.062,-.028,.007,.042,.077,.10]):
    yy=-.414+abs(x)*.39;zz=2.283
    length=[.041,.052,.029,.034,.043,.034,.046][i]
    loft('Upper_tooth', [((x,yy,zz),.013,.014),((x,yy-.004,zz-length),.0035,.004)],'bone','head',5)
for i,x in enumerate([-.09,-.056,.044,.083]):
    yy=-.414+abs(x)*.3
    loft('Lower_tooth',[((x,yy,2.10),.013,.012),((x,yy-.002,2.131+(i%2)*.012),.003,.003)],'bone','jaw',5)
# Thin surface fissures in the forehead, and short cracks beside the eye.
mesh('Forehead_fissure',[(-.025,-.277,2.738),(-.012,-.28,2.728),(.006,-.317,2.661),(-.004,-.323,2.622),(.015,-.336,2.575),(.006,-.339,2.566),(-.014,-.324,2.620),(-.005,-.317,2.663)],[(0,1,2,7),(7,2,3,6),(6,3,4,5)],'crack')
for i,(x,length) in enumerate([(-.033,.15),(0,.245),(.031,.18),(.055,.11)]):
    loft('Toxic_drool_'+str(i),[((x,-.441,2.112),.017,.012),((x+.008,-.453,2.076),.015,.011),((x+.004,-.449,2.112-length),.002,.002)],'toxic','jaw',5)
    if i==1: ellipsoid('Drool_bead',(x+.004,-.449,2.112-length-.027),(.009,.008,.02),'toxic','jaw',1)

# Helper bones use the Unreal names and live under the single root.
bone('ik_foot_root',(0,0,0),(0,0,.12),'root',False)
bone('ik_hand_root',(0,0,0),(0,0,.12),'root',False)
bone('ik_hand_gun',BONES['hand_r'][0],BONES['hand_r'][1],'ik_hand_root',False)
for side in ['l','r']:
    bone('ik_foot_'+side,*BONES['foot_'+side][:2],'ik_foot_root',False)
    bone('ik_hand_'+side,*BONES['hand_'+side][:2],'ik_hand_gun',False)

# Repair skull UV/weights after the boolean operations.
for g in list(skull.vertex_groups): skull.vertex_groups.remove(g)
for uv in list(skull.data.uv_layers): skull.data.uv_layers.remove(uv)
paint(skull,'bone');bind(skull,'head')

arm=bpy.data.armatures.new('SK_PlagueGhoul')
rig=bpy.data.objects.new('Armature',arm);scene.collection.objects.link(rig)
bpy.context.view_layer.objects.active=rig;rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for name,(a,b,parent,deform) in BONES.items():
    eb=arm.edit_bones.new(name);eb.head=a*S;eb.tail=b*S;eb.use_deform=deform
    if parent: eb.parent=arm.edit_bones[parent]
    eb.align_roll(Vector((0,1,0)))
bpy.ops.object.mode_set(mode='OBJECT');rig.show_in_front=True
bpy.ops.object.select_all(action='DESELECT')
for obj in PARTS: obj.select_set(True)
bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join()
character=bpy.context.object;character.name='SKM_PlagueGhoul';character.data.name='PlagueGhoul_LowPoly'
# Apply final triangulation, preserve deliberate flat face normals.
bm=bmesh.new();bm.from_mesh(character.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(character.data);bm.free()
modifier=character.modifiers.new('Skeleton','ARMATURE');modifier.object=rig
character.parent=rig
character['reference']='User-supplied low-poly undead reference; unseen surfaces interpreted.'
character['bind_pose']='A-pose, arms 45 degrees below shoulder line'
rig['retarget']='UE5 Manny core naming; use own Skeleton and IK Retargeter'

def select_model():
    bpy.ops.object.select_all(action='DESELECT');character.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig

select_model()
bpy.ops.export_scene.fbx(filepath=str(OUT/'SKM_PlagueGhoul_APose.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,use_armature_deform_only=False,armature_nodetype='NULL',primary_bone_axis='Y',secondary_bone_axis='X',bake_anim=False,path_mode='COPY',embed_textures=True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'SKM_PlagueGhoul_APose.glb'),export_format='GLB',use_selection=True,export_yup=True,export_skins=True,export_animations=False,export_materials='EXPORT',export_all_influences=False)

stats={'name':'Plague Ghoul','vertices':len(character.data.vertices),'triangles':len(character.data.polygons),'bones':len(BONES),'height_cm':round(character.dimensions.z,2),'width_cm':round(character.dimensions.x,2),'material_slots':len(character.material_slots),'unweighted_vertices':sum(not v.groups for v in character.data.vertices),'max_influences':max(len(v.groups) for v in character.data.vertices),'bones_hierarchy':{k:v[2] for k,v in BONES.items()}}
stats['revision']='2026-10-21 normal arm proportions'
stats['proportions']={'arm_length_scale':ARM_LENGTH_SCALE,'hand_scale':HAND_SCALE,'upperarm_cm':round((BONES['upperarm_l'][1]-BONES['upperarm_l'][0]).length*S,3),'forearm_cm':round((BONES['lowerarm_l'][1]-BONES['lowerarm_l'][0]).length*S,3)}
assert stats['unweighted_vertices']==0,stats
assert stats['max_influences']<=4,stats
assert [b.name for b in arm.bones if b.parent is None]==['root']
(OUT/'model_report.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
print('MODEL_REPORT',json.dumps({k:v for k,v in stats.items() if k!='bones_hierarchy'}),flush=True)

# Neutral studio, saved separately from the exported selection.
scene.render.engine='CYCLES';scene.cycles.samples=40;scene.cycles.use_denoising=True
scene.render.resolution_x=1400;scene.render.resolution_y=1400;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.world.color=(.2,.2,.2)
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.16,.18,.20,1);scene.world.node_tree.nodes['Background'].inputs[1].default_value=.45
scene.view_settings.view_transform='AgX'
groundmat=bpy.data.materials.new('Studio_floor');groundmat.diffuse_color=(.085,.095,.105,1);groundmat.use_nodes=True;groundmat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.085,.095,.105,1);groundmat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.95
bpy.ops.mesh.primitive_plane_add(size=20000,location=(0,0,-.6));ground=bpy.context.object;ground.name='STUDIO_Ground';ground.data.materials.append(groundmat)
def point_at(obj,target):obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
for name,loc,power,size,color in [('Key',(-350,-450,600),4000000,400,(1,.88,.72)),('Fill',(400,-150,350),2000000,350,(.7,.82,1)),('Rim',(80,350,500),3800000,300,(.90,1,.79))]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color
    light=bpy.data.objects.new(name,data);scene.collection.objects.link(light);light.location=loc;point_at(light,(0,0,140))
data=bpy.data.cameras.new('PreviewCamera');camera=bpy.data.objects.new('PreviewCamera',data);scene.collection.objects.link(camera);scene.camera=camera;data.type='ORTHO';data.ortho_scale=385;data.clip_end=10000
camera.location=(390,-720,330);point_at(camera,(0,-1,141))
scene.render.filepath=str(OUT/'Preview_ThreeQuarter.png')
select_model()
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_distance=460
            area.spaces.active.region_3d.view_location=(0,0,140)
            area.spaces.active.clip_end=10000
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'PlagueGhoul_APose.blend'))
bpy.ops.render.render(write_still=True)
camera.location=(0,-850,143);point_at(camera,(0,0,143));data.ortho_scale=385
scene.render.filepath=str(OUT/'Preview_Front.png');bpy.ops.render.render(write_still=True)
camera.location=(390,720,320);point_at(camera,(0,0,143));data.ortho_scale=385
scene.render.filepath=str(OUT/'Preview_Back.png');bpy.ops.render.render(write_still=True)
print('BUILD_COMPLETE',str(OUT),flush=True)
