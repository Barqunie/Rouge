"""Reproducible low-poly Needle assassin, A-pose bind skeleton, FBX/GLB and studio renders.
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
bpy.context.preferences.filepaths.save_version = 0

def bone(name, head, tail, parent=None, deform=True):
    BONES[name] = (Vector(head), Vector(tail), parent, deform)

PALETTE = {
 'skin': (187,143,105), 'cloth':(48,44,39), 'scarf':(132,43,36),
 'leather':(76,56,42), 'metal':(73,78,61), 'shadow':(19,23,19),
 'toxic':(166,238,28), 'trim':(113,93,66)
}
palette = Image.new('RGBA',(256,256))
pix=palette.load()
for row,(label,rgb) in enumerate(PALETTE.items()):
    for col in range(8):
        f=[.76,.84,.91,.97,1.0,1.04,1.09,1.14][col]
        color=tuple(min(255,round(x*f)) for x in rgb)+(255,)
        for y in range(row*32,(row+1)*32):
            for x in range(col*32,(col+1)*32): pix[x,y]=color
palette_path=OUT/'T_Needle_Palette.png'
palette.save(palette_path)
tex=bpy.data.images.load(str(palette_path))
tex.pack()
materials=[]
for glow in [False,True]:
    mat=bpy.data.materials.new('M_Needle_Toxic' if glow else 'M_Needle_Body')
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
        col=random.choices(range(8),weights=[0,1,2,7,20,9,2,0])[0] if variation else 4
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

# New geometry, authored from the supplied Needle concept. Front is -Y in Blender.
bone('root',(0,0,0),(0,0,.12))
bone('pelvis',(0,0,.96),(0,0,1.065),'root')
spine=[(0,0,1.065),(0,0,1.145),(0,.005,1.235),(0,.008,1.325),(0,.005,1.415),(0,0,1.505)]
for i in range(5): bone(f'spine_{i+1:02}',spine[i],spine[i+1],'pelvis' if i==0 else f'spine_{i:02}')
bone('neck_01',spine[-1],(0,0,1.55),'spine_05')
bone('neck_02',(0,0,1.55),(0,-.01,1.595),'neck_01')
bone('head',(0,-.01,1.595),(0,-.01,1.85),'neck_02')
bone('jaw',(0,-.045,1.66),(0,-.105,1.59),'head')
body_w=chain_weights(['pelvis']+[f'spine_{i:02}' for i in range(1,6)]+['neck_01'],.045)

def panel(name,points,color,weights,thickness=.006):
    obj=mesh(name,points,[tuple(range(len(points)))],color,weights)
    if thickness:
        bpy.context.view_layer.objects.active=obj
        mod=obj.modifiers.new('Cloth thickness','SOLIDIFY');mod.thickness=thickness*S
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj

def box(name,center,size,color,weights='pelvis'):
    c=Vector(center);d=Vector(size)*.5
    vs=[c+Vector((x*d.x,y*d.y,z*d.z)) for x,y,z in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
    return mesh(name,vs,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],color,weights)

def ring_band(name,a,b,rw,rd,color,weights,sides=8):
    return loft(name,[(a,rw,rd),(b,rw,rd)],color,weights,sides)

# Fitted sleeveless padded vest, high neck, offset leather front panels.
zloft('Sleeveless_dark_tunic',[(.925,.165,.104,0),(1.01,.169,.107,0),(1.16,.14,.092,0),(1.32,.177,.115,.005),(1.44,.199,.113,.002),(1.50,.161,.10,0),(1.535,.065,.068,0)],'cloth',body_w,12)
loft('Neck',[((0,0,1.485),.063,.062),((0,-.005,1.60),.062,.058)],'skin',body_w,8)
for s in [-1,1]:
    panel('Vest_panel',[(s*.012,-.119,1.385),(s*.142,-.106,1.40),(s*.16,-.108,1.30),(s*.105,-.101,1.16),(s*.018,-.103,1.18)],'metal',body_w,.005)
    panel('Lower_tunic_flap',[(s*.014,-.115,1.06),(s*.155,-.095,1.075),(s*.166,-.105,.92),(s*.082,-.136,.87)],'cloth','pelvis',.009)
    panel('Shoulder_binding',[(s*.115,-.081,1.50),(s*.19,-.056,1.495),(s*.20,-.062,1.435),(s*.16,-.10,1.405)],'leather',body_w,.008)

HAND_FRAMES={}
for s,side in [(1,'l'),(-1,'r')]:
    shoulder=Vector((s*.235,0,1.485));elbow=Vector((s*.45,-.018,1.27));wrist=Vector((s*.646,-.025,1.074))
    axis=(wrist-elbow).normalized();across=Vector((s*.7071068,0,.7071068))
    hand_end=wrist+axis*.092
    bone('clavicle_'+side,(s*.033,0,1.475),shoulder,'spine_05')
    bone('upperarm_'+side,shoulder,elbow,'clavicle_'+side)
    bone('lowerarm_'+side,elbow,wrist,'upperarm_'+side)
    bone('hand_'+side,wrist,hand_end,'lowerarm_'+side)
    for base,a,b in [('upperarm',shoulder,elbow),('lowerarm',elbow,wrist)]:
        for n,t in [(1,.35),(2,.7)]:bone(f'{base}_twist_{n:02}_{side}',a.lerp(b,t),a.lerp(b,t+.17),base+'_'+side)
    arm_w=chain_weights(['clavicle_'+side,'upperarm_'+side,'lowerarm_'+side,'hand_'+side],.035)
    loft('Bare_arm_'+side,[((s*.17,0,1.475),.05,.05),(shoulder,.057,.055),(shoulder.lerp(elbow,.20),.054,.052),(shoulder.lerp(elbow,.5),.047,.044),(shoulder.lerp(elbow,.9),.036,.035),(elbow,.037,.037),(elbow.lerp(wrist,.2),.044,.038),(elbow.lerp(wrist,.5),.04,.033),(elbow.lerp(wrist,.83),.027,.026),(wrist,.025,.024)],'skin',arm_w,8)
    # Wrapped forearm, reinforced leather plates and fingerless gloves.
    for j,(t0,t1,rw,rd) in enumerate([(.35,.55,.043,.036),(.57,.74,.036,.031),(.77,.94,.031,.029)]):
        ring_band(f'Bracer_wrap_{side}_{j}',elbow.lerp(wrist,t0),elbow.lerp(wrist,t1),rw,rd,'leather',arm_w)
    loft('Bracer_plate_'+side,[(elbow.lerp(wrist,.4)+Vector((0,-.031,0)),.026,.013),(elbow.lerp(wrist,.62)+Vector((0,-.029,0)),.027,.012),(elbow.lerp(wrist,.88)+Vector((0,-.024,0)),.018,.012)],'cloth',arm_w,5)
    ring_band('Wrist_cuff_'+side,wrist-axis*.025,wrist+axis*.017,.034,.029,'cloth','hand_'+side)
    loft('Gloved_palm_'+side,[(wrist,.027,.025),(wrist.lerp(hand_end,.5),.042,.028),(hand_end,.042,.023)],'leather','hand_'+side,8)
    panel('Glove_back_'+side,[wrist+across*.022+Vector((0,-.027,0)),hand_end+across*.036+Vector((0,-.025,0)),hand_end-across*.036+Vector((0,-.025,0)),wrist-across*.022+Vector((0,-.027,0))],'cloth','hand_'+side,.004)
    for idx,(f,off,length) in enumerate([('index',-.029,.079),('middle',-.008,.088),('ring',.014,.081),('pinky',.034,.063)]):
        start=hand_end+across*off
        spread=across*((idx-1.5)*.004)
        a=start+axis*(length*.42)+spread+Vector((0,.001,0))
        b=start+axis*(length*.75)+spread+Vector((0,.006,0))
        tip=start+axis*length+spread+Vector((0,.013,0))
        meta=f'{f}_metacarpal_{side}';bone(meta,wrist+across*off*.6,start,'hand_'+side)
        chain=[]
        for k,(pa,pb) in enumerate([(start,a),(a,b),(b,tip)]):
            bn=f'{f}_{k+1:02}_{side}';bone(bn,pa,pb,meta if k==0 else chain[-1]);chain.append(bn)
        fw=chain_weights(chain,.009)
        loft('Finger_'+f+'_'+side,[(start,.010,.011),(a,.0105,.0105),(b,.009,.009),(tip,.007,.007)],'skin',fw,6)
        loft('Glove_finger_'+f+'_'+side,[(start,.0115,.012),(start.lerp(a,.68),.011,.011)],'leather',fw,6)
    thumb_start=wrist+axis*.035-across*.029
    a=thumb_start-across*.027+axis*.015;b=a-across*.008+axis*.026;tip=b+axis*.022+Vector((0,.008,0))
    thumb=[]
    for k,(pa,pb) in enumerate([(thumb_start,a),(a,b),(b,tip)]):
        bn=f'thumb_{k+1:02}_{side}';bone(bn,pa,pb,'hand_'+side if k==0 else thumb[-1]);thumb.append(bn)
    loft('Thumb_'+side,[(thumb_start,.016,.014),(a,.013,.013),(b,.011,.01),(tip,.008,.008)],'skin',chain_weights(thumb,.01),6)
    loft('Thumb_glove_'+side,[(thumb_start,.018,.016),(a,.015,.014)],'leather',thumb[0],6)
    grip=wrist+axis*.112+Vector((0,.032,0))
    # Accessory bones use +Y along the blade in Blender; exported with character.
    blade_dir=-across
    bone('weapon_'+side,grip,grip+blade_dir*.10,'hand_'+side,True)
    HAND_FRAMES[side]=(grip,blade_dir,axis)
    hip=Vector((s*.112,0,.96));knee=Vector((s*.17,-.025,.52));ankle=Vector((s*.188,.005,.105));ball=Vector((s*.188,-.14,.055))
    bone('thigh_'+side,hip,knee,'pelvis');bone('calf_'+side,knee,ankle,'thigh_'+side)
    bone('foot_'+side,ankle,ball,'calf_'+side);bone('ball_'+side,ball,(s*.188,-.255,.047),'foot_'+side)
    for base,a,b in [('thigh',hip,knee),('calf',knee,ankle)]:
        for n,t in [(1,.35),(2,.7)]:bone(f'{base}_twist_{n:02}_{side}',a.lerp(b,t),a.lerp(b,t+.17),base+'_'+side)
    leg_w=chain_weights(['thigh_'+side,'calf_'+side,'foot_'+side],.045)
    loft('Trouser_leg_'+side,[(hip+Vector((0,0,.048)),.129,.127),(hip.lerp(knee,.2),.138,.145),(hip.lerp(knee,.52),.146,.142),(hip.lerp(knee,.82),.133,.122),(knee+Vector((0,0,.015)),.106,.10),(knee.lerp(ankle,.1),.078,.077)],'cloth',leg_w,10)
    loft('Turned_trouser_cuff_'+side,[(knee+Vector((0,0,.045)),.133,.128),(knee-Vector((0,0,.02)),.12,.117)],'leather',leg_w,8)
    loft('Boot_shaft_'+side,[(knee.lerp(ankle,.08),.073,.071),(knee.lerp(ankle,.28),.076,.071),(knee.lerp(ankle,.63),.053,.051),(ankle,.047,.053)],'leather',leg_w,8)
    panel('Shin_plate_'+side,[(s*.17-.047,-.071,.45),(s*.17+.048,-.071,.45),(s*.187+.034,-.054,.205),(s*.187-.035,-.054,.205)],'cloth',leg_w,.009)
    for j,t in enumerate([.20,.63,.85]):
        c=knee.lerp(ankle,t);r=.08 if j==0 else (.059 if j==1 else .054)
        ring_band('Boot_strap_'+side+str(j),c+Vector((0,0,.022)),c-Vector((0,0,.022)),r,r*.92,'cloth',leg_w,8)
        box('Boot_buckle_'+side+str(j),(c.x+s*r*.85,-.025,c.z),(.015,.035,.029),'metal',leg_w)
    fw=chain_weights(['foot_'+side,'ball_'+side],.028)
    # Broad bevelled toe and distinct solid sole, both following the toe pivot.
    loft('Boot_'+side,[((s*.188,.068,.07),.056,.058),((s*.188,.006,.088),.065,.083),((s*.188,-.09,.073),.082,.061),((s*.188,-.19,.059),.091,.049),((s*.188,-.26,.048),.083,.035)],'leather',fw,8)
    loft('Boot_sole_'+side,[((s*.188,.067,.022),.06,.019),((s*.188,-.08,.022),.083,.019),((s*.188,-.20,.022),.094,.019),((s*.188,-.27,.022),.082,.019)],'cloth',fw,8)
    panel('Toe_cap_'+side,[(s*.188-.072,-.158,.105),(s*.188+.072,-.158,.105),(s*.188+.071,-.25,.076),(s*.188-.071,-.25,.076)],'trim',fw,.004)

# Slanted utility belt and poison cartridges, each rigidly bound to pelvis.
zloft('Utility_belt',[(1.047,.175,.117,0),(1.105,.171,.114,0)],'leather','pelvis',12,lambda i,j,a:.024*math.cos(a))
box('Belt_buckle',(0,-.123,1.076),(.064,.019,.051),'metal')
box('Buckle_inset',(0,-.136,1.076),(.039,.009,.028),'shadow')
box('Buckle_pin',(0,-.143,1.076),(.007,.007,.032),'trim')
for s in [-1,1]:
    vial_weight='thigh_l' if s==1 else 'thigh_r'
    for j in range(2):
        c=Vector((s*(.135+j*.048),-.137+j*.014,1.02-j*.008));d=Vector((s*.027,0,-.127))
        ring_band('Vial_pouch',c+d*.72+Vector((0,.005,0)),c+d*1.25+Vector((0,.005,0)),.027,.026,'leather',vial_weight,6)
        loft('Poison_vial',[(c+d*.15+Vector((0,-.034,0)),.019,.019),(c+d*.87+Vector((0,-.034,0)),.019,.019)],'toxic',vial_weight,6)
        ring_band('Vial_cap',c+Vector((0,-.034,0)),c+d*.18+Vector((0,-.034,0)),.024,.023,'metal',vial_weight,6)
        ring_band('Vial_base',c+d*.82+Vector((0,-.034,0)),c+d+Vector((0,-.034,0)),.023,.021,'metal',vial_weight,6)
    box('Belt_loop',(s*.102,-.112,1.08+s*.015),(.022,.015,.077),'cloth')
box('Rear_pouch',(.11,.133,1.019),(.11,.064,.115),'leather')
panel('Rear_pouch_flap',[(.05,.172,1.083),(.17,.172,1.083),(.16,.18,1.02),(.11,.183,1.00),(.061,.18,1.02)],'cloth','pelvis',.005)

# Angular face framed by a genuinely open, thick hood.
ellipsoid('Shadowed_head',(0,-.004,1.704),(.119,.098,.145),'skin','head',2)
panel('Face_shadow',[(-.108,-.079,1.77),(.108,-.079,1.77),(.105,-.097,1.65),(0,-.12,1.60),(-.105,-.097,1.65)],'shadow','head',.006)
front=[(-.103,-.084,1.567),(-.157,-.076,1.655),(-.158,-.107,1.782),(-.068,-.10,1.872),(.035,-.112,1.895),(.147,-.133,1.807),(.155,-.079,1.657),(.105,-.079,1.57)]
back=[(x*.86,.124 if z<1.83 else .07,z-.012) for x,y,z in front]
verts=front+back+[(0,.151,1.73)]
faces=[]
for i in range(8):
    j=(i+1)%8;faces.append((i,j,j+8,i+8));faces.append((i+8,j+8,16))
hood=mesh('Faceted_open_hood',verts,faces,'cloth','head')
bpy.context.view_layer.objects.active=hood
mod=hood.modifiers.new('Hood thickness','SOLIDIFY');mod.thickness=.009*S;bpy.ops.object.modifier_apply(modifier=mod.name)
# Brow overhang, folded seam and asymmetric hanging edges.
panel('Hood_overhang',[(-.158,-.107,1.782),(-.068,-.10,1.872),(.035,-.112,1.895),(.147,-.133,1.807),(.107,-.164,1.79),(.008,-.156,1.765),(-.127,-.139,1.735)],'cloth','head',.011)
for s in [-1,1]:
    panel('Hood_cheek_fold',[(s*.143,-.084,1.739),(s*.13,-.114,1.642),(s*.109,-.121,1.58),(s*.161,-.048,1.655)],'cloth','head',.007)
    # Two green lenses tucked under the hood, with charcoal polygonal housings.
    c=Vector((s*.050,-.112,1.724))
    ring_band('Eye_rim',c+Vector((0,.011,0)),c-Vector((0,.009,0)),.029,.026,'metal','head',8)
    ellipsoid('Toxic_lens',c-Vector((0,.011,0)),(.021,.011,.021),'toxic','head',2,True)
    panel('Angled_brow',[(s*.021,-.127,1.75),(s*.079,-.12,1.765),(s*.082,-.13,1.75),(s*.027,-.134,1.739)],'cloth','head',.005)

ellipsoid('Respirator_body',(0,-.112,1.648),(.091,.052,.066),'metal','head',2)
panel('Respirator_nose',[(-.031,-.145,1.703),(.031,-.145,1.703),(.04,-.17,1.661),(0,-.187,1.652),(-.04,-.17,1.661)],'cloth','head',.007)
for s in [-1,1]:
    # Side filters are layered octagonal cylinders with inset perforated caps.
    c=Vector((s*.071,-.141,1.639));end=c+Vector((s*.013,-.045,-.007))
    ring_band('Filter_casing',c,end,.039,.037,'shadow','head',10)
    ring_band('Filter_rim',end,end+Vector((s*.002,-.009,0)),.035,.033,'metal','head',10)
    ring_band('Filter_face',end+Vector((0,-.009,0)),end+Vector((0,-.011,0)),.027,.026,'cloth','head',10)
    for dx in [-.010,0,.010]:
        loft('Filter_slit',[((end.x+dx,end.y-.013,end.z-.014),.0022,.0022),((end.x+dx,end.y-.013,end.z+.014),.0022,.0022)],'shadow','head',4)
    loft('Mask_cheek_strap',[((s*.081,-.104,1.667),.012,.008),((s*.114,-.05,1.677),.012,.007),((s*.11,.025,1.69),.012,.007)],'leather','head',5)
c=Vector((0,-.162,1.612))
ring_band('Central_valve',c,c+Vector((0,-.022,0)),.027,.025,'shadow','head',8)
ring_band('Central_valve_cap',c+Vector((0,-.022,0)),c+Vector((0,-.028,0)),.019,.018,'metal','head',8)

# Woven red cowl and pointed triangular front bib.
zloft('Scarf_collar',[(1.471,.129,.099,0),(1.51,.146,.11,0),(1.58,.093,.084,.006)],'scarf',body_w,10,lambda i,j,a:-.025*max(0,-math.sin(a)) if i==0 else 0)
panel('Scarf_bib',[(-.182,-.071,1.527),(-.113,-.122,1.562),(.162,-.108,1.492),(.12,-.146,1.42),(-.012,-.165,1.369),(-.129,-.141,1.443)],'scarf',body_w,.012)
panel('Scarf_fold',[(-.182,-.081,1.537),(-.13,-.145,1.462),(-.015,-.177,1.405),(.143,-.133,1.463),(.166,-.118,1.50),(-.016,-.162,1.448),(-.108,-.126,1.559)],'scarf',body_w,.006)
panel('Scarf_shoulder_left',[(.086,-.091,1.565),(.211,-.055,1.515),(.244,.07,1.478),(.162,.162,1.43),(.061,.102,1.53)],'scarf',body_w,.01)
panel('Scarf_shoulder_right',[(-.109,-.071,1.557),(-.221,-.034,1.516),(-.234,.092,1.475),(-.163,.159,1.432),(-.047,.11,1.534)],'scarf',body_w,.01)

# Three independently skinned, layered cape tails. Extra bones are available for
# AnimDynamics/Control Rig; no cloth simulation is silently configured.
for label,x0,x1,endz,width in [('l',.13,.29,.49,.118),('c',0,-.018,.65,.132),('r',-.13,-.29,.56,.121)]:
    centers=[Vector((x0,.145,1.49)),Vector((x0*.9,.245,1.20)),Vector((x1*.80,.285,.92)),Vector((x1,.34,endz))]
    chain=[]
    for i in range(3):
        n=f'cape_{label}_{i+1:02}';bone(n,centers[i],centers[i+1],'spine_05' if i==0 else chain[-1]);chain.append(n)
    vs=[]
    for i,c in enumerate(centers):
        w=width*[.65,1,1.05,.93][i]
        for j in [-1,0,1]:
            q=c+Vector((j*w,(.015 if j==0 else 0),0))
            if i==3:q.z += [.12,-.12,.19][j+1]
            vs.append(q)
    fs=[]
    for i in range(3):
        for j in range(2):fs.append((i*3+j,i*3+j+1,(i+1)*3+j+1,(i+1)*3+j))
    obj=mesh('Torn_cape_'+label,vs,fs,'scarf',chain_weights(chain,.075))
    bpy.context.view_layer.objects.active=obj
    mod=obj.modifiers.new('Cape thickness','SOLIDIFY');mod.thickness=.006*S;bpy.ops.object.modifier_apply(modifier=mod.name)

bone('ik_foot_root',(0,0,0),(0,0,.12),'root',False)
bone('ik_hand_root',(0,0,0),(0,0,.12),'root',False)
bone('ik_hand_gun',BONES['hand_r'][0],BONES['hand_r'][1],'ik_hand_root',False)
for side in ['l','r']:
    bone('ik_foot_'+side,*BONES['foot_'+side][:2],'ik_foot_root',False)
    bone('ik_hand_'+side,*BONES['hand_'+side][:2],'ik_hand_gun',False)

arm=bpy.data.armatures.new('SK_Needle')
rig=bpy.data.objects.new('Armature',arm);scene.collection.objects.link(rig)
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.object.mode_set(mode='EDIT')
for name,(a,b,parent,deform) in BONES.items():
    eb=arm.edit_bones.new(name);eb.head=a*S;eb.tail=b*S;eb.use_deform=deform
    if parent:eb.parent=arm.edit_bones[parent]
    eb.align_roll(Vector((0,1,0)))
bpy.ops.object.mode_set(mode='OBJECT');rig.show_in_front=True
bpy.ops.object.select_all(action='DESELECT')
for obj in PARTS:obj.select_set(True)
bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join()
character=bpy.context.object;character.name='SKM_Needle';character.data.name='Needle_LowPoly'
bm=bmesh.new();bm.from_mesh(character.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(character.data);bm.free()
modifier=character.modifiers.new('Skeleton','ARMATURE');modifier.object=rig;character.parent=rig
character['reference']='Needle concept supplied by user; unseen surfaces interpreted.'
character['bind_pose']='A pose; symmetric normal human arm proportions'
rig['retarget']='Own skeleton, UE5 Manny core names, IK Retargeter'

def export_fbx(path,objects,types):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:obj.select_set(True)
    bpy.context.view_layer.objects.active=objects[-1]
    bpy.ops.export_scene.fbx(filepath=str(OUT/path),use_selection=True,object_types=types,global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,use_armature_deform_only=False,armature_nodetype='NULL',primary_bone_axis='Y',secondary_bone_axis='X',bake_anim=False,path_mode='COPY',embed_textures=True)

export_fbx('SKM_Needle_APose.fbx',[character,rig],{'MESH','ARMATURE'})
bpy.ops.export_scene.gltf(filepath=str(OUT/'SKM_Needle_APose.glb'),export_format='GLB',use_selection=True,export_yup=True,export_skins=True,export_animations=False,export_materials='EXPORT')

# Separate dagger: grip centre at origin, blade extends local +Y, +Z is spine.
PARTS=[]
ring_band('Dagger_grip',(0,-.053,0),(0,.048,0),.016,.017,'leather','root',8)
for y in [-.045,-.025,-.005,.015,.035]:ring_band('Grip_binding',(0,y-.003,0),(0,y+.003,0),.018,.019,'cloth','root',8)
box('Dagger_guard',(0,.055,0),(.099,.025,.036),'metal','root')
ring_band('Dagger_pommel',(0,-.068,0),(0,-.053,0),.023,.024,'metal','root',8)
mesh('Poisoned_blade',[(-.032,.068,0),(.032,.068,0),(-.018,.22,0),(.018,.22,0),(0,.382,0),(0,.075,.012),(0,.075,-.012),(0,.24,.007),(0,.24,-.007)],[(0,5,7,2),(2,7,4),(5,1,3,7),(7,3,4),(0,2,8,6),(2,4,8),(6,8,3,1),(8,4,3),(0,6,1,5)],'toxic','root',True)
mesh('Dagger_steel_spine',[(-.006,.07,.012),(.006,.07,.012),(.003,.225,.008),(0,.355,.002),(-.003,.225,.008)],[(0,1,2,4),(4,2,3)],'metal','root')
bpy.ops.object.select_all(action='DESELECT')
for o in PARTS:o.select_set(True)
bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join()
dagger=bpy.context.object;dagger.name='SM_Needle_Dagger'
for group in list(dagger.vertex_groups):dagger.vertex_groups.remove(group)
export_fbx('SM_Needle_Dagger.fbx',[dagger],{'MESH'})
# Include two separately skinned daggers in an optional equipped FBX.
dagger.hide_render=True;dagger.hide_viewport=True
equipped=[]
from mathutils import Matrix
for side in ['l','r']:
    grip,blade_dir,across=HAND_FRAMES[side]
    obj=dagger.copy();obj.data=dagger.data.copy();scene.collection.objects.link(obj)
    obj.name='Dagger_'+side;obj.hide_viewport=False;obj.hide_render=True
    rot=rig.data.bones['weapon_'+side].matrix_local.to_3x3()
    for v in obj.data.vertices:v.co=rot@v.co+grip*S
    bind(obj,'weapon_'+side)
    mod=obj.modifiers.new('Skeleton','ARMATURE');mod.object=rig;obj.parent=rig
    equipped.append(obj)
export_fbx('SKM_Needle_Equipped_APose.fbx',[character]+equipped+[rig],{'MESH','ARMATURE'})

stats={'name':'The Needle','vertices':len(character.data.vertices),'triangles':len(character.data.polygons),'bones':len(BONES),'height_cm':round(character.dimensions.z,2),'width_cm':round(character.dimensions.x,2),'material_slots':len(character.material_slots),'unweighted_vertices':sum(not v.groups for v in character.data.vertices),'max_influences':max(len(v.groups) for v in character.data.vertices),'bones_hierarchy':{k:v[2] for k,v in BONES.items()},'upperarm_cm':round((BONES['upperarm_l'][1]-BONES['upperarm_l'][0]).length*S,3),'forearm_cm':round((BONES['lowerarm_l'][1]-BONES['lowerarm_l'][0]).length*S,3),'cape_bones':9,'dagger_triangles':sum(len(p.vertices)-2 for p in dagger.data.polygons)}
assert stats['unweighted_vertices']==0
assert stats['max_influences']<=4
assert [b.name for b in arm.bones if not b.parent]==['root']
(OUT/'model_report.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
print('MODEL_REPORT',json.dumps({k:v for k,v in stats.items() if k!='bones_hierarchy'}),flush=True)

# Studio scene is not part of any exported game mesh.
scene.render.engine='CYCLES';scene.cycles.samples=40;scene.cycles.use_denoising=True
scene.render.resolution_x=1400;scene.render.resolution_y=1400;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.13,.16,.19,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.5
scene.view_settings.view_transform='AgX'
groundmat=bpy.data.materials.new('Studio_floor');groundmat.use_nodes=True
groundmat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.065,.075,.08,1)
groundmat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.95
bpy.ops.mesh.primitive_plane_add(size=20000,location=(0,0,-.1));ground=bpy.context.object;ground.name='STUDIO_Ground';ground.data.materials.append(groundmat)
def point_at(obj,target):obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
for name,loc,power,size,color in [('Key',(-250,-300,400),2200000,260,(1,.87,.71)),('Fill',(300,-200,260),1500000,240,(.74,.86,1)),('Rim',(120,250,360),2400000,220,(.84,1,.76))]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color
    light=bpy.data.objects.new(name,data);scene.collection.objects.link(light);light.location=loc;point_at(light,(0,0,95))
data=bpy.data.cameras.new('PreviewCamera');camera=bpy.data.objects.new('PreviewCamera',data);scene.collection.objects.link(camera);scene.camera=camera
data.type='ORTHO';data.ortho_scale=231;data.clip_end=10000
camera.location=(300,-650,240);point_at(camera,(0,0,96))
bpy.ops.object.select_all(action='DESELECT');character.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_distance=290
            area.spaces.active.region_3d.view_location=(0,0,96)
            area.spaces.active.clip_end=10000
scene.render.filepath=str(OUT/'Preview_ThreeQuarter.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Needle_APose.blend'))
bpy.ops.render.render(write_still=True)
camera.location=(0,-700,97);point_at(camera,(0,0,97))
scene.render.filepath=str(OUT/'Preview_Front.png');bpy.ops.render.render(write_still=True)
camera.location=(300,650,240);point_at(camera,(0,0,96))
scene.render.filepath=str(OUT/'Preview_Back.png');bpy.ops.render.render(write_still=True)
print('BUILD_COMPLETE',str(OUT),flush=True)

