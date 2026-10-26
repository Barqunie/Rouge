"""Reproducible low-poly Bulk chempunk tank, A-pose bind skeleton, FBX/GLB and studio renders.
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
 'skin':(169,121,96), 'cloth':(43,40,36), 'armor':(59,57,49),
 'leather':(71,57,43), 'steel':(113,109,96), 'shadow':(23,25,20),
 'toxic':(168,223,15), 'hazard':(156,128,83)
}
palette = Image.new('RGBA',(256,256))
pix=palette.load()
for row,(label,rgb) in enumerate(PALETTE.items()):
    for col in range(8):
        f=[.76,.84,.91,.97,1.0,1.04,1.09,1.14][col]
        color=tuple(min(255,round(x*f)) for x in rgb)+(255,)
        for y in range(row*32,(row+1)*32):
            for x in range(col*32,(col+1)*32): pix[x,y]=color
palette_path=OUT/'T_Bulk_Palette.png'
palette.save(palette_path)
tex=bpy.data.images.load(str(palette_path))
tex.pack()
materials=[]
for glow in [False,True]:
    mat=bpy.data.materials.new('M_Bulk_Toxic' if glow else 'M_Bulk_Body')
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

def ribbon(name,centers,width,color,weights,thickness=.012,across=(1,0,0)):
    u=Vector(across).normalized()*width*.5
    verts=[]
    for point in centers:
        c=Vector(point)
        for a,b in [(-1,-1),(1,-1),(1,1),(-1,1)]:verts.append(c+u*a+Vector((0,b*thickness*.5,0)))
    faces=[(3,2,1,0)]
    for i in range(len(centers)-1):
        for j in range(4):faces.append((i*4+j,i*4+(j+1)%4,(i+1)*4+(j+1)%4,(i+1)*4+j))
    faces.append(tuple((len(centers)-1)*4+j for j in range(4)))
    return mesh(name,verts,faces,color,weights)

def rivet(name,center,weight,size=.009):
    return ellipsoid(name,center,(size,size*.45,size),'steel',weight,1)

def hazard_mark(name,center,size,color,weight,right=(1,0,0),up=(0,0,1)):
    c=Vector(center);u=Vector(right)*size;v=Vector(up)*size
    outer=[c+u*x+v*z for x,z in [(0,.57),(-.50,-.30),(.50,-.30)]]
    inner=[c+(p-c)*.71 for p in outer]
    mesh(name+'_triangle',outer+inner,[(0,1,4,3),(1,2,5,4),(2,0,3,5)],color,weight)
    origin=c-v*.035
    for sector in range(3):
        vs=[]
        for rad in [.09,.235]:
            for k in range(4):
                a=math.radians(18+sector*120+k*24)
                vs.append(origin+(u*math.cos(a)+v*math.sin(a))*rad)
        mesh(name+'_trefoil',vs,[(0,1,5,4),(1,2,6,5),(2,3,7,6)],color,weight)
    vs=[origin]+[origin+(u*math.cos(a*math.tau/8)+v*math.sin(a*math.tau/8))*.052 for a in range(8)]
    mesh(name+'_dot',vs,[(0,j+1,(j+1)%8+1) for j in range(8)],color,weight)

# Broad, compact humanoid skeleton, authored in a neutral A pose.
bone('root',(0,0,0),(0,0,.15))
bone('pelvis',(0,0,1.00),(0,0,1.10),'root')
spine=[(0,0,1.10),(0,0,1.22),(0,.006,1.34),(0,.012,1.46),(0,.012,1.58),(0,0,1.70)]
for i in range(5):bone(f'spine_{i+1:02}',spine[i],spine[i+1],'pelvis' if i==0 else f'spine_{i:02}')
bone('neck_01',spine[-1],(0,-.006,1.755),'spine_05')
bone('neck_02',(0,-.006,1.755),(0,-.015,1.82),'neck_01')
bone('head',(0,-.015,1.82),(0,-.015,2.108),'neck_02')
bone('jaw',(0,-.05,1.89),(0,-.14,1.80),'head')
bone('tank',(0,.30,1.36),(0,.30,1.91),'spine_04')
bone('apron_01',(0,-.218,1.055),(0,-.249,.84),'pelvis')
bone('apron_02',(0,-.249,.84),(0,-.266,.64),'apron_01')
body_w=chain_weights(['pelvis']+[f'spine_{i:02}' for i in range(1,6)]+['neck_01'],.06)

# Faceted torso with low-relief pectoral, abdominal and back muscles.
zloft('Muscular_torso',[(.96,.27,.175,0),(1.075,.282,.191,0),(1.21,.293,.197,.003),(1.37,.351,.226,.01),(1.55,.398,.23,.012),(1.67,.376,.206,.012),(1.745,.251,.151,0),(1.79,.115,.098,-.006)],'skin',body_w,16)
loft('Thick_neck',[((0,0,1.65),.15,.12),((0,-.006,1.76),.119,.10),((0,-.015,1.85),.113,.093)],'skin',body_w,10)
for s in [-1,1]:
    ellipsoid('Pectoral',(s*.196,-.184,1.56),(.204,.078,.127),'skin',body_w,2)
    for z,rx,ry,rz,cy in [(1.34,.116,.055,.083,-.207),(1.22,.109,.049,.078,-.193),(1.11,.102,.042,.066,-.185)]:
        ellipsoid('Abdominal',(s*(rx*.9),cy+.025,z),(rx,ry*.70,rz),'skin',body_w,1)
    ellipsoid('Latissimus',(s*.25,.159,1.47),(.13,.055,.211),'skin',body_w,2)
    loft('Trapezius',[((s*.095,.004,1.782),.081,.083),((s*.24,.025,1.726),.11,.115),((s*.367,.022,1.69),.12,.12)],'skin',body_w,8)

for s,side in [(1,'l'),(-1,'r')]:
    shoulder=Vector((s*.423,.005,1.675));elbow=Vector((s*.682,-.026,1.404));wrist=Vector((s*.914,-.04,1.16))
    axis=(wrist-elbow).normalized();across=Vector((s*.724,0,.689)).normalized();hand_end=wrist+axis*.13
    bone('clavicle_'+side,(s*.045,.005,1.70),shoulder,'spine_05')
    bone('upperarm_'+side,shoulder,elbow,'clavicle_'+side)
    bone('lowerarm_'+side,elbow,wrist,'upperarm_'+side)
    bone('hand_'+side,wrist,hand_end,'lowerarm_'+side)
    for base,a,b in [('upperarm',shoulder,elbow),('lowerarm',elbow,wrist)]:
        for n,t in [(1,.35),(2,.70)]:bone(f'{base}_twist_{n:02}_{side}',a.lerp(b,t),a.lerp(b,t+.17),base+'_'+side)
    arm_w=chain_weights(['clavicle_'+side,'upperarm_'+side,'lowerarm_'+side,'hand_'+side],.06)
    loft('Massive_arm_'+side,[((s*.31,.005,1.68),.135,.139),(shoulder,.164,.155),(shoulder.lerp(elbow,.22),.162,.147),(shoulder.lerp(elbow,.49),.139,.129),(shoulder.lerp(elbow,.74),.113,.107),(elbow,.087,.086),(elbow.lerp(wrist,.19),.119,.107),(elbow.lerp(wrist,.47),.111,.099),(elbow.lerp(wrist,.77),.085,.077),(wrist,.067,.062)],'skin',arm_w,10)
    ellipsoid('Deltoid_'+side,shoulder+Vector((s*.006,-.013,.015)),(.175,.169,.162),'skin',arm_w,2)
    bicep=shoulder.lerp(elbow,.44)+Vector((0,-.063,0))
    ellipsoid('Bicep_'+side,bicep,(.116,.106,.129),'skin',arm_w,1)
    # Armoured, open-ended bracers leave the elbows free to bend.
    loft('Forearm_gauntlet_'+side,[(elbow.lerp(wrist,.26),.126,.115),(elbow.lerp(wrist,.55),.123,.109),(elbow.lerp(wrist,.9),.094,.083)],'armor',arm_w,8)
    for j,t in enumerate([.29,.79]):
        c=elbow.lerp(wrist,t);rw=.131 if j==0 else .107;rd=.12 if j==0 else .094
        ring_band('Bracer_band_'+side+str(j),c-axis*.026,c+axis*.026,rw,rd,'leather',arm_w,8)
    a=elbow.lerp(wrist,.36)+Vector((0,-.113,0));b=elbow.lerp(wrist,.72)+Vector((0,-.102,0))
    panel('Bracer_face_'+side,[a-across*.085,a+across*.085,b+across*.070,b-across*.070],'armor',arm_w,.018)
    for off in [-.041,0,.041]:
        loft('Green_bracer_slot_'+side,[(a+across*off+Vector((0,-.014,0)),.010,.008),(b+across*off+Vector((0,-.014,0)),.008,.006)],'toxic',arm_w,5)
    for p in [a-across*.067,a+across*.067,b-across*.052,b+across*.052]:rivet('Bracer_rivet',p+Vector((0,-.022,0)),arm_w,.008)
    ring_band('Glove_cuff_'+side,wrist-axis*.018,wrist+axis*.033,.083,.071,'cloth','hand_'+side,8)
    loft('Fingerless_glove_'+side,[(wrist,.068,.063),(wrist.lerp(hand_end,.46),.092,.064),(hand_end,.098,.051)],'cloth','hand_'+side,8)
    panel('Glove_reinforcement_'+side,[wrist+across*.048+Vector((0,-.065,0)),hand_end+across*.071+Vector((0,-.052,0)),hand_end-across*.071+Vector((0,-.052,0)),wrist-across*.048+Vector((0,-.065,0))],'leather','hand_'+side,.009)
    for idx,(finger,off,length) in enumerate([('index',-.065,.122),('middle',-.021,.139),('ring',.025,.13),('pinky',.068,.103)]):
        start=hand_end+across*off;spread=across*((idx-1.5)*.006)
        a=start+axis*(length*.40)+spread+Vector((0,.003,0));b=start+axis*(length*.74)+spread+Vector((0,.014,0));tip=start+axis*length+spread+Vector((0,.027,0))
        meta=f'{finger}_metacarpal_{side}';bone(meta,wrist+across*off*.65,start,'hand_'+side)
        chain=[]
        for k,(pa,pb) in enumerate([(start,a),(a,b),(b,tip)]):
            name=f'{finger}_{k+1:02}_{side}';bone(name,pa,pb,meta if k==0 else chain[-1]);chain.append(name)
        fw=chain_weights(chain,.016)
        loft('Finger_'+finger+'_'+side,[(start,.023,.024),(a,.022,.022),(b,.020,.020),(tip,.015,.017)],'skin',fw,6)
        loft('Glove_finger_'+finger+'_'+side,[(start,.025,.026),(start.lerp(a,.60),.024,.024)],'cloth',fw,6)
    start=wrist+axis*.035-across*.065;a=start-across*.049+axis*.025;b=a-across*.018+axis*.044;tip=b+axis*.042+Vector((0,.012,0))
    thumb=[]
    for k,(pa,pb) in enumerate([(start,a),(a,b),(b,tip)]):
        name=f'thumb_{k+1:02}_{side}';bone(name,pa,pb,'hand_'+side if k==0 else thumb[-1]);thumb.append(name)
    loft('Thumb_'+side,[(start,.034,.029),(a,.031,.029),(b,.025,.024),(tip,.019,.019)],'skin',chain_weights(thumb,.016),6)
    loft('Thumb_glove_'+side,[(start,.037,.032),(start.lerp(a,.67),.034,.031)],'cloth',thumb[0],6)
    hip=Vector((s*.188,0,1.0));knee=Vector((s*.255,-.027,.53));ankle=Vector((s*.289,.008,.14));ball=Vector((s*.289,-.178,.068))
    bone('thigh_'+side,hip,knee,'pelvis');bone('calf_'+side,knee,ankle,'thigh_'+side)
    bone('foot_'+side,ankle,ball,'calf_'+side);bone('ball_'+side,ball,(s*.289,-.33,.055),'foot_'+side)
    for base,a,b in [('thigh',hip,knee),('calf',knee,ankle)]:
        for n,t in [(1,.35),(2,.7)]:bone(f'{base}_twist_{n:02}_{side}',a.lerp(b,t),a.lerp(b,t+.17),base+'_'+side)
    leg_w=chain_weights(['thigh_'+side,'calf_'+side,'foot_'+side],.06)
    loft('Heavy_trouser_'+side,[(hip+Vector((0,0,.073)),.185,.183),(hip.lerp(knee,.22),.198,.195),(hip.lerp(knee,.55),.188,.185),(hip.lerp(knee,.83),.164,.165),(knee,.146,.148),(knee.lerp(ankle,.30),.151,.147),(knee.lerp(ankle,.55),.119,.119),(knee.lerp(ankle,.71),.092,.093)],'cloth',leg_w,10)
    # Side cargo pockets, thigh straps and protected knees.
    for j,t in enumerate([.23,.55]):
        c=hip.lerp(knee,t)
        ring_band('Thigh_strap_'+side+str(j),c+Vector((0,0,.025)),c-Vector((0,0,.025)),.202 if j==0 else .192,.199 if j==0 else .190,'leather',leg_w,10)
    pocket=hip.lerp(knee,.34)+Vector((s*.112,-.142,0))
    box('Cargo_pocket_'+side,pocket,(.15,.07,.18),'armor','thigh_'+side)
    panel('Cargo_flap_'+side,[pocket+Vector((-.079,-.041,.095)),pocket+Vector((.079,-.041,.095)),pocket+Vector((.069,-.045,.037)),pocket+Vector((-.063,-.045,.029))],'leather','thigh_'+side,.01)
    c=knee+Vector((0,-.145,.007))
    panel('Knee_plate_'+side,[c+Vector((-.125,0,.104)),c+Vector((.115,0,.111)),c+Vector((.13,-.02,-.065)),c+Vector((.045,-.035,-.117)),c+Vector((-.116,-.014,-.082))],'armor',leg_w,.035)
    panel('Knee_inner_plate_'+side,[c+Vector((-.073,-.027,.063)),c+Vector((.074,-.027,.067)),c+Vector((.069,-.044,-.050)),c+Vector((-.074,-.040,-.058))],'leather',leg_w,.007)
    for dx,dz in [(-.086,.080),(.084,.082),(-.085,-.06),(.09,-.05)]:rivet('Knee_rivet',c+Vector((dx,-.03,dz)),leg_w,.007)
    loft('Boot_shaft_'+side,[(knee.lerp(ankle,.55),.105,.108),(knee.lerp(ankle,.74),.098,.099),(ankle,.09,.102)],'armor',leg_w,8)
    c=knee.lerp(ankle,.70)
    ring_band('Boot_toxic_band_'+side,c+Vector((0,0,.022)),c-Vector((0,0,.022)),.115,.119,'toxic',leg_w,8)
    for t in [.56,.90]:
        c=knee.lerp(ankle,t)
        ring_band('Boot_fastening_'+side,c+Vector((0,0,.024)),c-Vector((0,0,.024)),.112,.117,'leather',leg_w,8)
    fw=chain_weights(['foot_'+side,'ball_'+side],.04)
    loft('Steel_boot_'+side,[((s*.289,.09,.095),.102,.084),((s*.289,-.015,.115),.12,.105),((s*.289,-.16,.089),.149,.074),((s*.289,-.285,.073),.155,.059),((s*.289,-.35,.055),.137,.043)],'armor',fw,8)
    loft('Heavy_boot_sole_'+side,[((s*.289,.091,.026),.107,.025),((s*.289,-.10,.026),.144,.025),((s*.289,-.28,.026),.162,.025),((s*.289,-.355,.026),.14,.025)],'shadow',fw,8)
    panel('Toe_armour_'+side,[(s*.289-.134,-.20,.144),(s*.289+.134,-.20,.144),(s*.289+.131,-.326,.102),(s*.289-.131,-.326,.102)],'leather',fw,.015)
    vs=[]
    for y,z,bump in [(-.19,.151,.034),(-.224,.139,.030)]:
        for f in [-1,-.55,0,.55,1]:vs.append((s*.289+f*.14,y,z+bump*(1-abs(f))))
    mesh('Green_toe_stripe_'+side,vs,[(j,j+1,j+6,j+5) for j in range(4)],'toxic',fw)
    for edge in [-1,1]:
        panel('Boot_side_signal',[(s*.289+edge*.145,-.241,.095),(s*.289+edge*.15,-.28,.08),(s*.289+edge*.145,-.295,.04),(s*.289+edge*.147,-.26,.045)],'toxic',fw,.004)

# Substantial belt, solid metal frame buckle, front apron and fluorescent vials.
zloft('Trouser_waist',[(.91,.354,.21,0),(1.02,.335,.211,0),(1.084,.287,.204,0)],'cloth','pelvis',14)
zloft('Waist_belt',[(1.035,.293,.201,0),(1.115,.293,.201,0)],'leather','pelvis',14)
box('Belt_buckle_back',(0,-.214,1.075),(.134,.025,.092),'shadow')
for x,z,w,h in [(-.061,1.075,.017,.103),(.061,1.075,.017,.103),(0,1.12,.121,.017),(0,1.03,.121,.017)]:
    box('Steel_belt_buckle',(x,-.237,z),(w,.02,h),'steel')
box('Buckle_tongue',(-.025,-.253,1.075),(.085,.013,.014),'steel')
for s in [-1,1]:
    box('Belt_keeper',(s*.155,-.177,1.075),(.031,.025,.115),'armor')
    for j in range(2):
        side='l' if s==1 else 'r';w='thigh_'+side
        c=Vector((s*(.242+j*.066),-.177+j*.013,1.06-j*.007));d=Vector((s*.025,0,-.17))
        ring_band('Vial_cradle',c+d*.76+Vector((0,.01,0)),c+d*1.1+Vector((0,.01,0)),.038,.032,'leather',w,8)
        obj=loft('Toxic_vial',[(c+d*.12+Vector((0,-.02,0)),.027,.026),(c+d*.9+Vector((0,-.02,0)),.027,.026)],'toxic',w,8)
        ring_band('Vial_top',c+Vector((0,-.02,0)),c+d*.17+Vector((0,-.02,0)),.035,.032,'armor',w,8)
        ring_band('Vial_bottom',c+d*.86+Vector((0,-.02,0)),c+d*1.04+Vector((0,-.02,0)),.034,.032,'armor',w,8)
        ring_band('Vial_steel_ring',c+d*.14+Vector((0,-.02,0)),c+d*.20+Vector((0,-.02,0)),.030,.029,'steel',w,8)
apron_w=chain_weights(['apron_01','apron_02'],.055)
panel('Hanging_hazard_apron',[(-.135,-.224,1.04),(.135,-.224,1.04),(.145,-.25,.86),(.152,-.273,.66),(-.148,-.273,.65),(-.14,-.25,.86)],'leather',apron_w,.014)
ribbon('Apron_left_hem',[(-.128,-.235,1.03),(-.135,-.261,.86),(-.143,-.285,.665)],.013,'armor',apron_w,.006)
ribbon('Apron_right_hem',[(.128,-.235,1.03),(.14,-.261,.86),(.146,-.285,.665)],.013,'armor',apron_w,.006)
hazard_mark('Apron_hazard',(0,-.286,.792),.205,'hazard',apron_w)

# Backpack harness and riveted shoulder pads.
for s in [-1,1]:
    centers=[(s*.25,.235,1.40),(s*.278,.235,1.62),(s*.286,.13,1.76),(s*.287,-.02,1.782),(s*.286,-.16,1.72),(s*.286,-.269,1.57),(s*.265,-.279,1.42),(s*.213,-.238,1.23)]
    ribbon('Tank_shoulder_strap',centers,.087,'cloth',body_w,.023)
    panel('Riveted_shoulder_plate',[(s*.235,-.158,1.73),(s*.351,-.139,1.726),(s*.373,-.007,1.792),(s*.291,.101,1.809),(s*.223,.07,1.793)],'armor',body_w,.031)
    panel('Shoulder_front_plate',[(s*.239,-.188,1.708),(s*.347,-.178,1.70),(s*.338,-.269,1.582),(s*.247,-.281,1.576)],'armor',body_w,.018)
    for x,y,z in [(s*.251,-.196,1.69),(s*.331,-.19,1.686),(s*.257,-.291,1.595),(s*.326,-.281,1.603)]:rivet('Shoulder_plate_bolt',(x,y,z),body_w,.009)
    box('Strap_buckle',(s*.265,-.295,1.463),(.092,.027,.071),'steel',body_w)
    box('Strap_buckle_inset',(s*.265,-.313,1.463),(.063,.012,.045),'cloth',body_w)
    box('Strap_buckle_bar',(s*.265,-.324,1.463),(.070,.01,.013),'leather',body_w)
ribbon('Diagonal_chest_belt',[(-.276,-.286,1.584),(-.08,-.288,1.448),(.10,-.276,1.317),(.227,-.237,1.204)],.084,'leather',body_w,.025,across=(.55,0,.83))
ribbon('Cross_chest_belt',[(.274,-.281,1.58),(.075,-.300,1.445),(-.092,-.273,1.311),(-.229,-.237,1.204)],.072,'cloth',body_w,.026,across=(.55,0,-.83))
ellipsoid('Chest_regulator',(0,-.276,1.311),(.078,.04,.084),'armor',body_w,1)
box('Regulator_plate',(0,-.315,1.32),(.080,.022,.07),'leather',body_w)
loft('Regulator_toxic_gauge',[((.034,-.331,1.294),.010,.007),((.034,-.331,1.341),.010,.007)],'toxic',body_w,5)
zloft('Reinforced_neck_collar',[(1.737,.14,.114,-.004),(1.775,.136,.109,-.004)],'armor','neck_01',10)

# One rigid chemical canister with a visible lime window, structural rails,
# clamp bands, valves and a hazard marking on its rear face.
box('Backpack_support',(0,.238,1.56),(.35,.075,.58),'shadow','tank')
loft('Tank_housing',[((0,.369,1.30),.13,.13),((0,.369,1.35),.17,.16),((0,.369,1.86),.17,.16),((0,.369,1.92),.13,.125)],'armor','tank',12)
loft('Toxic_tank_window',[((0,.369,1.41),.171,.162),((0,.369,1.795),.171,.162)],'toxic','tank',12)
for z0,z1,rw,rd in [(1.33,1.405,.181,.172),(1.795,1.873,.181,.172),(1.57,1.605,.176,.168)]:
    ring_band('Tank_clamp',(0,.369,z0),(0,.369,z1),rw,rd,'armor','tank',12)
for x,y in [(-.145,.45),(.145,.45),(-.145,.285),(.145,.285)]:
    box('Tank_vertical_rail',(x,y,1.609),(.036,.036,.436),'armor','tank')
ring_band('Tank_top_cap',(0,.369,1.865),(0,.369,1.935),.137,.132,'leather','tank',10)
ring_band('Tank_valve_stem',(0,.369,1.929),(0,.369,1.99),.032,.032,'steel','tank',8)
ring_band('Tank_valve_wheel',(0,.369,1.984),(0,.369,2.006),.071,.068,'armor','tank',8)
ring_band('Tank_bottom_drain',(0,.369,1.255),(0,.369,1.32),.045,.045,'armor','tank',8)
hazard_mark('Tank_hazard',(0,.535,1.675),.155,'armor','tank',right=(-1,0,0))
# A deliberately solid green window avoids transparency sorting on a low-poly asset.
panel('Tank_liquid_glint',[(-.069,.532,1.622),(-.044,.532,1.622),(-.044,.532,1.774),(-.069,.532,1.774)],'hazard','tank',.001)

def hose_w(p):
    t=max(0,min(1,(p.y+.10)/.30))
    return {'tank':t,'spine_05':1-t}
for s in [-1,1]:
    path=[(s*.11,.38,1.844),(s*.205,.28,1.839),(s*.285,.14,1.805),(s*.33,-.018,1.759),(s*.325,-.148,1.684),(s*.305,-.244,1.60),(s*.254,-.285,1.56),(s*.209,-.281,1.568)]
    loft('Lime_pressure_hose_'+str(s),[(p,.029,.029) for p in path],'toxic',hose_w,8)
    a=Vector(path[0]);b=Vector(path[1]);ring_band('Tank_hose_socket',a-(b-a)*.12,a+(b-a)*.30,.043,.041,'armor','tank',8)
    a=Vector(path[-1]);b=Vector(path[-2]);ring_band('Chest_hose_socket',a-(b-a)*.30,a+(b-a)*.38,.046,.044,'armor','spine_05',8)
    ring_band('Chest_hose_lock',a,a+(b-a)*.18,.048,.046,'steel','spine_05',8)

# Bald angular head. The dark sockets and sloped brow carry the expression,
# while the lower face is enclosed by the twin-filter respirator.
zloft('Bald_cranium',[(1.80,.085,.075,-.025),(1.87,.135,.105,-.014),(1.96,.148,.119,-.007),(2.045,.137,.111,0),(2.096,.096,.084,.002),(2.12,.043,.047,0)],'skin','head',12)
for s in [-1,1]:
    ellipsoid('Ear',(s*.145,.004,1.926),(.035,.032,.056),'skin','head',1)
    ellipsoid('Inner_ear',(s*.159,-.023,1.927),(.013,.009,.029),'leather','head',1)
    ellipsoid('Dark_eye_socket',(s*.064,-.115,1.973),(.044,.022,.032),'shadow','head',1)
    eye=[(s*.024,-.141,1.977),(s*.095,-.132,1.991),(s*.081,-.142,1.961),(s*.047,-.147,1.960)]
    eye_obj=mesh('Angry_toxic_eye',eye,[(0,1,2,3)],'toxic','head',True)
    panel('Lower_eyelid',[(s*.043,-.145,1.955),(s*.085,-.139,1.959),(s*.105,-.122,1.974),(s*.083,-.134,1.948)],'skin','head',.004)
    mesh('Heavy_angular_brow',[(s*.017,-.152,1.988),(s*.098,-.135,2.023),(s*.119,-.105,2.01),(s*.028,-.14,1.978),(s*.034,-.106,2.028),(s*.094,-.10,2.045)],[(0,1,2,3),(0,4,5,1),(1,5,2),(3,2,5,4),(0,3,4)],'leather','head')
    loft('Respirator_head_strap',[((s*.117,-.10,1.90),.018,.012),((s*.15,-.019,1.902),.018,.011),((s*.132,.078,1.91),.018,.011)],'armor','head',6)
ellipsoid('Respirator_core',(0,-.139,1.866),(.119,.081,.094),'armor','head',2)
panel('Armoured_nose',[(-.027,-.156,1.97),(.027,-.156,1.97),(.055,-.205,1.898),(.035,-.234,1.857),(-.035,-.234,1.857),(-.055,-.205,1.898)],'armor','head',.018)
for s in [-1,1]:
    c=Vector((s*.099,-.185,1.854));front=c+Vector((s*.013,-.055,0))
    ring_band('Respirator_filter',c,front,.062,.065,'shadow','head',10)
    ring_band('Respirator_filter_rim',front,front+Vector((0,-.014,0)),.059,.061,'armor','head',10)
    ring_band('Respirator_filter_face',front+Vector((0,-.014,0)),front+Vector((0,-.019,0)),.046,.05,'cloth','head',10)
    for j in [-1,0,1]:
        p=front+Vector((j*.018,-.022,0));dz=.031 if j==0 else .023
        loft('Lime_filter_vent',[(p+Vector((0,0,-dz)),.0045,.004),(p+Vector((0,0,dz)),.0045,.004)],'toxic','head',5)
    panel('Mask_cheek_ridge',[(s*.088,-.193,1.943),(s*.139,-.147,1.935),(s*.157,-.164,1.88),(s*.126,-.218,1.874)],'armor','head',.008)
loft('Chin_seal',[((-.087,-.131,1.816),.023,.02),((0,-.194,1.787),.024,.02),((.087,-.131,1.816),.023,.02)],'shadow','head',8)
# Forehead scar: narrow dark geometry resting on the faceted scalp.
panel('Forehead_scar',[(-.056,-.086,2.083),(-.052,-.09,2.079),(-.076,-.105,2.052),(-.063,-.115,2.029),(-.068,-.115,2.026),(-.083,-.103,2.053)],'leather','head',.001)

# Standard mannequin helper names; only one root in the exported skeleton.
bone('ik_foot_root',(0,0,0),(0,0,.12),'root',False)
bone('ik_hand_root',(0,0,0),(0,0,.12),'root',False)
bone('ik_hand_gun',BONES['hand_r'][0],BONES['hand_r'][1],'ik_hand_root',False)
for side in ['l','r']:
    bone('ik_foot_'+side,*BONES['foot_'+side][:2],'ik_foot_root',False)
    bone('ik_hand_'+side,*BONES['hand_'+side][:2],'ik_hand_gun',False)

# Unite the torso/arm muscle volumes before final skinning. This removes
# intersecting shells at the deltoids and chest while retaining a faceted surface.
skin_prefixes=('Muscular_torso','Thick_neck','Pectoral','Abdominal','Latissimus','Trapezius','Massive_arm_','Deltoid_','Bicep_')
skin_parts=[o for o in PARTS if o.name.startswith(skin_prefixes)]
PARTS=[o for o in PARTS if o not in skin_parts]
bpy.ops.object.select_all(action='DESELECT')
for o in skin_parts:o.select_set(True)
bpy.context.view_layer.objects.active=skin_parts[0];bpy.ops.object.join()
skin=bpy.context.object;skin.name='Unified_muscular_skin'
skin.data.remesh_voxel_size=1.8;skin.data.use_remesh_preserve_volume=True
bpy.ops.object.voxel_remesh()
smooth=skin.modifiers.new('Volume fusion','SMOOTH');smooth.factor=.5;smooth.iterations=2
bpy.ops.object.modifier_apply(modifier=smooth.name)
skin.data.calc_loop_triangles()
dec=skin.modifiers.new('Low poly surface','DECIMATE');dec.ratio=min(1.,2800/len(skin.data.loop_triangles));dec.use_collapse_triangulate=True
bpy.ops.object.modifier_apply(modifier=dec.name)
for group in list(skin.vertex_groups):skin.vertex_groups.remove(group)
skin_arm_w={side:chain_weights(['clavicle_'+side,'upperarm_'+side,'lowerarm_'+side,'hand_'+side],.06) for side in ['l','r']}
def unified_w(p):
    a=body_w(p);b=skin_arm_w['l' if p.x>0 else 'r'](p)
    t=max(0,min(1,(abs(p.x)-.30)/.12))
    result={n:a.get(n,0)*(1-t)+b.get(n,0)*t for n in set(a)|set(b)}
    return {n:w for n,w in sorted(result.items(),key=lambda i:-i[1])[:4] if w>1e-8}
paint(skin,'skin');bind(skin,unified_w);PARTS.append(skin)

# Fit the front harness to the fused chest rather than leaving the straps on a
# flat plane. Keep strap thickness and the two crossing layers distinct.
from mathutils.bvhtree import BVHTree
bpy.context.view_layer.update()
surface=BVHTree.FromObject(skin,bpy.context.evaluated_depsgraph_get())
for obj in PARTS:
    ribbon_clearance=next((clear for prefix,clear in [('Tank_shoulder_strap',1.8),('Diagonal_chest_belt',1.8),('Cross_chest_belt',3.4)] if obj.name.startswith(prefix)),None)
    if ribbon_clearance is not None:
        vs=obj.data.vertices
        for i in range(0,len(vs),4):
            group=list(vs[i:i+4]);cy=sum(v.co.y for v in group)/4
            if cy>=-13:continue
            for v in group:
                hit,normal,index,distance=surface.ray_cast(Vector((v.co.x,-1000,v.co.z)),Vector((0,1,0)),2000)
                if hit is not None:v.co.y=hit.y-ribbon_clearance+(v.co.y-cy)
        obj.data.update()
    elif obj.name.startswith('Strap_buckle'):
        center=sum((v.co for v in obj.data.vertices),Vector())/len(obj.data.vertices)
        hit,normal,index,distance=surface.ray_cast(Vector((center.x,-1000,center.z)),Vector((0,1,0)),2000)
        if hit is not None:
            clearance=5.9 if 'bar' in obj.name else (4.8 if 'inset' in obj.name else 3.0)
            dy=hit.y-clearance-center.y
            for v in obj.data.vertices:v.co.y+=dy
            obj.data.update()

arm=bpy.data.armatures.new('SK_Bulk')
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
character=bpy.context.object;character.name='SKM_Bulk';character.data.name='Bulk_LowPoly'
bm=bmesh.new();bm.from_mesh(character.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(character.data);bm.free()
mod=character.modifiers.new('Skeleton','ARMATURE');mod.object=rig;character.parent=rig
character['reference']='The Bulk chempunk tank, user supplied reference sheet; one neutral A-pose model.'
rig['retarget']='Own skeleton, UE5 Manny core names, use IK Retargeter'

def select_model():
    bpy.ops.object.select_all(action='DESELECT');character.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig

select_model()
bpy.ops.export_scene.fbx(filepath=str(OUT/'SKM_Bulk_APose.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,use_armature_deform_only=False,armature_nodetype='NULL',primary_bone_axis='Y',secondary_bone_axis='X',bake_anim=False,path_mode='COPY',embed_textures=True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'SKM_Bulk_APose.glb'),export_format='GLB',use_selection=True,export_yup=True,export_skins=True,export_animations=False,export_materials='EXPORT')
stats={'name':'The Bulk','vertices':len(character.data.vertices),'triangles':len(character.data.polygons),'bones':len(BONES),'height_cm':round(character.dimensions.z,2),'width_cm':round(character.dimensions.x,2),'material_slots':len(character.material_slots),'unweighted_vertices':sum(not v.groups for v in character.data.vertices),'max_influences':max(len(v.groups) for v in character.data.vertices),'bones_hierarchy':{k:v[2] for k,v in BONES.items()},'upperarm_cm':round((BONES['upperarm_l'][1]-BONES['upperarm_l'][0]).length*S,3),'forearm_cm':round((BONES['lowerarm_l'][1]-BONES['lowerarm_l'][0]).length*S,3),'bind_pose':'A pose only; no concept action poses authored'}
assert stats['unweighted_vertices']==0 and stats['max_influences']<=4
assert [b.name for b in arm.bones if not b.parent]==['root']
(OUT/'model_report.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
print('MODEL_REPORT',json.dumps({k:v for k,v in stats.items() if k!='bones_hierarchy'}),flush=True)

# Neutral studio for shape review: three viewpoints, the same single A pose.
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
for name,loc,power,size,color in [('Key',(-280,-350,440),2800000,290,(1,.87,.75)),('Fill',(320,-190,310),1800000,260,(.75,.86,1)),('Rim',(100,280,400),2800000,250,(.84,1,.75))]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color
    obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);obj.location=loc;point_at(obj,(0,0,110))
data=bpy.data.cameras.new('PreviewCamera');camera=bpy.data.objects.new('PreviewCamera',data);scene.collection.objects.link(camera);scene.camera=camera
data.type='ORTHO';data.ortho_scale=290;data.clip_end=10000
camera.location=(330,-700,280);point_at(camera,(0,0,108))
select_model()
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_distance=360
            area.spaces.active.region_3d.view_location=(0,0,108)
            area.spaces.active.clip_end=10000
scene.render.filepath=str(OUT/'Preview_ThreeQuarter.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Bulk_APose.blend'))
bpy.ops.render.render(write_still=True)
camera.location=(0,-800,109);point_at(camera,(0,0,109));scene.render.filepath=str(OUT/'Preview_Front.png');bpy.ops.render.render(write_still=True)
camera.location=(330,700,280);point_at(camera,(0,0,108));scene.render.filepath=str(OUT/'Preview_Back.png');bpy.ops.render.render(write_still=True)
print('BUILD_COMPLETE',str(OUT),flush=True)

