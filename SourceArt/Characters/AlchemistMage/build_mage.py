"""Reproducible low-poly alchemist mage, A-pose bind skeleton, FBX/GLB and studio renders.
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
random.seed(64)
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

PALETTE = {'skin':(224,176,150),'ivory':(224,218,198),'leather':(49,41,39),'hair':(118,19,55),'gold':(169,127,64),'shadow':(25,23,25),'toxic':(161,218,19),'lining':(87,110,32)}
palette = Image.new('RGBA',(256,256))
pix=palette.load()
for row,(label,rgb) in enumerate(PALETTE.items()):
    for col in range(8):
        f=[.76,.84,.91,.97,1.0,1.04,1.09,1.14][col]
        color=tuple(min(255,round(x*f)) for x in rgb)+(255,)
        for y in range(row*32,(row+1)*32):
            for x in range(col*32,(col+1)*32): pix[x,y]=color
palette_path=OUT/'T_AlchemistMage_Palette.png'
palette.save(palette_path)
tex=bpy.data.images.load(str(palette_path))
tex.pack()
materials=[]
for glow in [False,True]:
    mat=bpy.data.materials.new('M_AlchemistMage_Toxic' if glow else 'M_AlchemistMage_Body')
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


# Slender human skeleton in A-pose; centimetres are applied by the mesh helper.
bone('root',(0,0,0),(0,0,.12))
bone('pelvis',(0,0,1.00),(0,0,1.10),'root')
spine=[(0,0,1.10),(0,0,1.18),(0,0,1.26),(0,0,1.34),(0,0,1.42),(0,0,1.51)]
for i in range(5):bone(f'spine_{i+1:02}',spine[i],spine[i+1],'pelvis' if i==0 else f'spine_{i:02}')
bone('neck_01',spine[-1],(0,0,1.555),'spine_05')
bone('neck_02',(0,0,1.555),(0,0,1.605),'neck_01')
bone('head',(0,0,1.605),(0,0,1.83),'neck_02')
bone('jaw',(0,-.025,1.665),(0,-.07,1.608),'head')
body_w=chain_weights(['pelvis']+[f'spine_{i:02}' for i in range(1,6)]+['neck_01'],.04)
zloft('Body',[(.96,.145,.088,0),(1.04,.16,.096,0),(1.14,.124,.081,0),(1.27,.115,.076,0),(1.37,.146,.092,0),(1.43,.164,.091,0),(1.49,.143,.074,0),(1.53,.058,.053,0)],'skin',body_w,12)
loft('Neck',[((0,0,1.49),.052,.047),((0,0,1.62),.047,.043)],'skin',body_w,10)
zloft('Dark_shorts',[(.95,.153,.098,0),(1.055,.162,.102,0),(1.10,.151,.103,0)],'leather','pelvis',12)
# One continuous faceted bodice; a sweetheart top edge and a fitted corset.
zloft('Ivory_bodice',[(1.286,.126,.096,-.004),(1.31,.137,.101,-.006),(1.34,.15,.108,-.007),(1.40,.159,.119,-.009),(1.448,.154,.104,-.006)],'ivory',body_w,16,lambda i,j,a:(-.034*max(0,-math.sin(a))*(1-abs(math.cos(a)))) if i==4 else 0)
zloft('Leather_corset',[(1.075,.153,.10,0),(1.14,.13,.093,0),(1.245,.122,.097,-.004),(1.30,.132,.102,-.004)],'leather',body_w,12)
zloft('High_ivory_collar',[(1.528,.058,.054,0),(1.603,.054,.050,0)],'ivory',body_w,10)
for s in [-1,1]:
    ribbon('Bodice_strap',[(s*.123,-.071,1.421),(s*.145,-.045,1.496),(s*.135,.045,1.49),(s*.115,.087,1.39)],.025,'leather',body_w,.007)
    box('Collar_gold_clasp',(s*.026,-.051,1.562),(.018,.007,.022),'gold','neck_02')
for z in [1.155,1.22,1.278]:
    box('Corset_buckle',(-.012,-.109,z),(.039,.009,.028),'gold',body_w)
    box('Corset_buckle_hole',(-.012,-.115,z),(.024,.005,.015),'shadow',body_w)
    box('Corset_pin',(-.012,-.119,z),(.004,.003,.02),'gold',body_w)


HAND_FRAMES={}
for s,side in [(1,'l'),(-1,'r')]:
    shoulder=Vector((s*.20,0,1.47));elbow=Vector((s*.397,-.018,1.27));wrist=Vector((s*.58,-.025,1.085))
    axis=(wrist-elbow).normalized();across=Vector((s*.7071068,0,.7071068))
    hand_end=wrist+axis*.083
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
    loft('Bracer_plate_'+side,[(elbow.lerp(wrist,.4)+Vector((0,-.031,0)),.026,.013),(elbow.lerp(wrist,.62)+Vector((0,-.029,0)),.027,.012),(elbow.lerp(wrist,.88)+Vector((0,-.024,0)),.018,.012)],'leather',arm_w,5)
    ring_band('Wrist_cuff_'+side,wrist-axis*.025,wrist+axis*.017,.034,.029,'leather','hand_'+side)
    loft('Gloved_palm_'+side,[(wrist,.027,.025),(wrist.lerp(hand_end,.5),.042,.028),(hand_end,.042,.023)],'leather','hand_'+side,8)
    panel('Glove_back_'+side,[wrist+across*.022+Vector((0,-.027,0)),hand_end+across*.036+Vector((0,-.025,0)),hand_end-across*.036+Vector((0,-.025,0)),wrist-across*.022+Vector((0,-.027,0))],'leather','hand_'+side,.004)
    for idx,(f,off,length) in enumerate([('index',-.029,.073),('middle',-.008,.080),('ring',.014,.075),('pinky',.034,.058)]):
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
        loft('Finger_'+f+'_'+side,[(start,.010,.011),(a,.0105,.0105),(b,.009,.009),(tip,.007,.007)],'leather',fw,6)
        loft('Glove_finger_'+f+'_'+side,[(start,.0115,.012),(start.lerp(a,.68),.011,.011)],'leather',fw,6)
    thumb_start=wrist+axis*.035-across*.029
    a=thumb_start-across*.027+axis*.015;b=a-across*.008+axis*.026;tip=b+axis*.022+Vector((0,.008,0))
    thumb=[]
    for k,(pa,pb) in enumerate([(thumb_start,a),(a,b),(b,tip)]):
        bn=f'thumb_{k+1:02}_{side}';bone(bn,pa,pb,'hand_'+side if k==0 else thumb[-1]);thumb.append(bn)
    loft('Thumb_'+side,[(thumb_start,.016,.014),(a,.013,.013),(b,.011,.01),(tip,.008,.008)],'leather',chain_weights(thumb,.01),6)
    loft('Thumb_glove_'+side,[(thumb_start,.018,.016),(a,.015,.014)],'leather',thumb[0],6)

    # White puff sleeve starts below an exposed shoulder. Long dark glove.
    loft('Puff_sleeve_'+side,[(shoulder.lerp(elbow,.19),.067,.062),(shoulder.lerp(elbow,.38),.076,.073),(shoulder.lerp(elbow,.65),.068,.062),(elbow,.048,.045),(elbow.lerp(wrist,.14),.052,.045)],'ivory',arm_w,10)
    ring_band('Sleeve_cuff_'+side,elbow.lerp(wrist,.10),elbow.lerp(wrist,.19),.051,.046,'ivory',arm_w,10)
    loft('Long_glove_'+side,[(elbow.lerp(wrist,.23),.046,.041),(elbow.lerp(wrist,.51),.042,.035),(wrist,.028,.026)],'leather',arm_w,8)
    ring_band('Glove_gold_cuff',elbow.lerp(wrist,.77),elbow.lerp(wrist,.83),.035,.031,'gold',arm_w,8)
    lens=wrist.lerp(hand_end,.40)+Vector((0,-.032,0))
    ellipsoid('Glove_gem_socket',lens,(.022,.012,.022),'gold','hand_'+side,1)
    ellipsoid('Glove_gem',lens+Vector((0,-.009,0)),(.012,.007,.013),'toxic','hand_'+side,1,True)
    hip=Vector((s*.093,0,1.025));knee=Vector((s*.132,-.025,.565));ankle=Vector((s*.151,.005,.13));ball=Vector((s*.151,-.12,.055))
    bone('thigh_'+side,hip,knee,'pelvis');bone('calf_'+side,knee,ankle,'thigh_'+side)
    bone('foot_'+side,ankle,ball,'calf_'+side);bone('ball_'+side,ball,(s*.151,-.219,.05),'foot_'+side)
    for base,a,b in [('thigh',hip,knee),('calf',knee,ankle)]:
        for n,t in [(1,.35),(2,.7)]:bone(f'{base}_twist_{n:02}_{side}',a.lerp(b,t),a.lerp(b,t+.17),base+'_'+side)
    lw=chain_weights(['thigh_'+side,'calf_'+side,'foot_'+side],.045)
    loft('Leg_'+side,[(hip+Vector((0,0,.025)),.096,.09),(hip.lerp(knee,.28),.095,.094),(hip.lerp(knee,.55),.077,.077),(knee,.054,.056),(knee.lerp(ankle,.3),.063,.055),(knee.lerp(ankle,.7),.042,.041),(ankle,.035,.037)],'skin',lw,10)
    loft('Thigh_high_boot_'+side,[(hip.lerp(knee,.36),.091,.092),(hip.lerp(knee,.62),.075,.077),(knee,.06,.061),(knee.lerp(ankle,.3),.065,.06),(knee.lerp(ankle,.72),.047,.046),(ankle,.041,.043)],'leather',lw,10)
    for j,t in enumerate([.10,.76]):
        c=knee.lerp(ankle,t);r=.067 if j==0 else .049
        ring_band('Boot_strap',c+Vector((0,0,.016)),c-Vector((0,0,.016)),r,r,'leather',lw,8)
        box('Boot_gold_buckle',(c.x+s*r*.76,-.035,c.z),(.027,.013,.036),'gold',lw)
        box('Boot_buckle_hole',(c.x+s*r*.76,-.043,c.z),(.014,.006,.021),'shadow',lw)
    fw=chain_weights(['foot_'+side,'ball_'+side],.025)
    loft('Heeled_boot_foot_'+side,[((s*.151,.048,.09),.043,.064),((s*.151,-.01,.095),.048,.068),((s*.151,-.09,.066),.058,.047),((s*.151,-.18,.047),.063,.035),((s*.151,-.228,.041),.053,.026)],'leather',fw,8)
    box('Boot_heel',(s*.151,.025,.033),(.061,.068,.06),'leather','foot_'+side)
    loft('Boot_toe_gold', [((s*.151,-.174,.023),.064,.016),((s*.151,-.228,.023),.056,.016)],'gold',fw,8)

zloft('Utility_belt',[(1.064,.161,.108,0),(1.107,.156,.106,0)],'leather','pelvis',12)
box('Belt_gold_buckle',(0,-.118,1.084),(.062,.014,.05),'gold')
box('Belt_buckle_inset',(0,-.128,1.084),(.04,.006,.03),'shadow')
box('Belt_pin',(0,-.133,1.084),(.005,.004,.035),'gold')
for s in [-1,1]:
    for j in range(2):
        c=Vector((s*(.108+j*.052),-.113+j*.028,1.041-j*.01))
        ring_band('Belt_vial',c-Vector((0,0,.055)),c+Vector((0,0,.047)),.020,.020,'toxic','pelvis',8)
        for dz in [-.055,.049]:ring_band('Vial_gold_cap',c+Vector((0,0,dz-.012)),c+Vector((0,0,dz+.012)),.025,.024,'gold','pelvis',8)
        box('Vial_bracket',c+Vector((0,-.020,0)),(.013,.013,.042),'leather')
box('Potion_pouch',(-.188,.012,.989),(.085,.081,.134),'leather')
box('Pouch_latch',(-.188,-.035,1.01),(.029,.010,.025),'gold')
ellipsoid('Round_flask',(.209,-.034,.964),(.056,.051,.065),'toxic','pelvis',2,True)
ring_band('Flask_neck',(.209,-.034,1.012),(.209,-.034,1.065),.019,.018,'gold','pelvis',8)
ring_band('Flask_band',(.209,-.034,.955),(.209,-.034,.975),.057,.052,'gold','pelvis',10)

# Four split coat tails with articulated three-bone chains. The centre stays open.
for label,points,width in [
 ('side_l',[(.142,.015,1.09),(.207,.035,.86),(.25,.07,.59),(.30,.09,.26)],.20),
 ('side_r',[(-.142,.015,1.09),(-.207,.035,.86),(-.25,.07,.59),(-.30,.09,.26)],.20),
 ('back_l',[(.07,.103,1.09),(.105,.17,.85),(.15,.22,.59),(.18,.265,.33)],.21),
 ('back_r',[(-.07,.103,1.09),(-.105,.17,.85),(-.15,.22,.59),(-.18,.265,.33)],.21)]:
    chain=[]
    for i in range(3):
        n=f'coat_{label}_{i+1:02}';bone(n,points[i],points[i+1],'pelvis' if i==0 else chain[-1]);chain.append(n)
    vs=[]
    for i,point in enumerate(points):
        for j in [-1,0,1]:
            q=Vector(point)+Vector((j*width*.5*(1+i*.16),-.018 if j==0 else 0,0))
            if i==3:q.z+=[.11,-.08,.15][j+1]
            vs.append(q)
    faces=[(i*3+j,i*3+j+1,(i+1)*3+j+1,(i+1)*3+j) for i in range(3) for j in range(2)]
    back=label.startswith('back')
    mesh('Coat_outer_'+label,vs,faces,'lining' if back else 'ivory',chain_weights(chain,.065))
    mesh('Coat_lining_'+label,[p+Vector((0,.008,0)) for p in vs],[tuple(reversed(f)) for f in faces],'ivory' if back else 'lining',chain_weights(chain,.065))
    # Narrow closed border around each panel avoids single-sided holes at hems.
    rim=[0,3,6,9,10,11,8,5,2,1]
    mesh('Coat_edge_'+label,vs+[p+Vector((0,.008,0)) for p in vs],[(rim[i],rim[(i+1)%len(rim)],rim[(i+1)%len(rim)]+12,rim[i]+12) for i in range(len(rim))],'ivory',chain_weights(chain,.065))
for s in [-1,1]:
    panel('Short_front_coat_flap',[(s*.025,-.115,1.09),(s*.139,-.087,1.09),(s*.181,-.07,.88),(s*.075,-.126,.973)],'ivory','pelvis',.006)

# Angular face: flattened facial planes, a small nose, real eye geometry and lips.
zloft('Face',[(1.601,.036,.041,-.015),(1.626,.064,.06,-.005),(1.667,.085,.073,0),(1.721,.090,.079,.003),(1.777,.085,.077,.007),(1.812,.065,.059,.012),(1.828,.031,.031,.012)],'skin','head',12)
for s in [-1,1]:
    mesh('Pointed_ear',[(s*.078,-.006,1.688),(s*.126,.008,1.738),(s*.104,.021,1.668),(s*.083,.025,1.67),(s*.102,-.008,1.697)],[(0,1,4),(1,2,4),(2,3,4),(3,0,4),(0,3,2,1)],'skin','head')
    panel('Ear_inner',[(s*.092,-.009,1.696),(s*.115,.004,1.725),(s*.102,.006,1.68)],'hair','head',.001)
    ring_band('Gold_earring',(s*.10,.006,1.672),(s*.103,.005,1.643),.007,.006,'gold','head',6)
# Viewer-left visible almond eye; the other is behind the monocle.
panel('Eye_socket',[(-.072,-.066,1.726),(-.047,-.082,1.734),(-.018,-.084,1.716),(-.041,-.085,1.704),(-.066,-.072,1.711)],'shadow','head',.001)
panel('Eye_white',[(-.068,-.070,1.721),(-.045,-.085,1.726),(-.022,-.087,1.716),(-.042,-.087,1.709),(-.062,-.077,1.713)],'ivory','head',.001)
ellipsoid('Iris',(-.042,-.088,1.717),(.009,.004,.009),'lining','head',2)
ellipsoid('Pupil',(-.042,-.092,1.717),(.0038,.002,.007),'shadow','head',1)
ellipsoid('Eye_glint',(-.045,-.094,1.721),(.0025,.0015,.0025),'ivory','head',1)
panel('Angular_eyebrow',[(-.074,-.066,1.747),(-.045,-.081,1.754),(-.018,-.084,1.738),(-.02,-.085,1.73),(-.048,-.082,1.742),(-.072,-.069,1.739)],'hair','head',.003)
mesh('Nose',[(-.009,-.078,1.727),(.009,-.078,1.727),(-.012,-.092,1.676),(.012,-.092,1.676),(0,-.111,1.683),(0,-.089,1.67)],[(0,1,4),(0,4,2),(1,3,4),(2,4,5),(4,3,5),(0,2,5,3,1)],'skin','head')
panel('Upper_lip',[(-.026,-.072,1.651),(-.009,-.081,1.657),(0,-.084,1.654),(.009,-.081,1.657),(.025,-.072,1.651),(0,-.085,1.647)],'hair','head',.002)
panel('Lower_lip',[(-.025,-.073,1.649),(0,-.087,1.646),(.025,-.073,1.649),(.012,-.08,1.640),(-.01,-.08,1.640)],'hair','head',.002)
c=Vector((.049,-.080,1.722))
ring_band('Monocle_dark_housing',c+Vector((0,.012,0)),c-Vector((0,.016,0)),.036,.036,'leather','head',10)
ring_band('Monocle_gold_rim',c-Vector((0,.015,0)),c-Vector((0,.025,0)),.031,.031,'gold','head',10)
ellipsoid('Toxic_monocle_lens',c-Vector((0,.026,0)),(.022,.010,.023),'toxic','head',2,True)
ellipsoid('Lens_glint',c+Vector((-.006,-.035,.008)),(.006,.002,.008),'ivory','head',1)
loft('Monocle_strap',[(c+Vector((.025,0,0)),.009,.007),((.096,-.011,1.724),.009,.007),((.074,.069,1.725),.009,.007)],'leather','head',6)

# Deliberately broad hair clumps rather than fine strands.
zloft('Hair_cap',[(1.738,.091,.080,.024),(1.782,.099,.086,.018),(1.822,.085,.076,.018),(1.846,.051,.051,.019)],'hair','head',12)
ellipsoid('Top_bun',(0,.055,1.872),(.067,.062,.058),'hair','head',2)
ring_band('Bun_gold_band',(-.054,.05,1.853),(.054,.05,1.881),.046,.045,'gold','head',8)
for s in [-1,1]:
    ring_band('Hair_stick',(s*.092,.055,1.837),(-s*.061,.055,1.913),.006,.006,'gold','head',6)
def hair_lock(name,points,widths,weight):
    return loft(name,[(p,w,w*.44) for p,w in zip(points,widths)],'hair',weight,5)
hair_lock('Swept_fringe',[(.070,-.047,1.819),(.028,-.089,1.801),(-.035,-.101,1.77),(-.085,-.083,1.735),(-.122,-.069,1.721)],[.047,.044,.034,.023,.001],'head')
hair_lock('Upper_fringe',[(.046,-.033,1.843),(-.023,-.078,1.82),(-.074,-.075,1.794),(-.103,-.056,1.771)],[.043,.036,.026,.001],'head')
hair_lock('Right_temple',[(.074,-.030,1.813),(.102,-.032,1.77),(.096,-.03,1.708),(.124,-.02,1.667)],[.033,.032,.022,.001],'head')
for side,s in [('l',1),('r',-1)]:
    pts=[(s*.062,.078,1.765),(s*.09,.137,1.56),(s*.13,.169,1.37),(s*.16,.175,1.17)]
    hc=[]
    for i in range(3):
        n=f'hair_{side}_{i+1:02}';bone(n,pts[i],pts[i+1],'head' if i==0 else hc[-1]);hc.append(n)
    hw=chain_weights(hc,.06)
    for j in range(4):
        dx=(j-1.5)*.025
        points=[(s*.065+dx,.069+j*.012,1.801-j*.012),(s*.113+dx,.126+j*.013,1.66-j*.018),(s*.083+dx,.174+j*.012,1.49-j*.022),(s*.16+dx,.182+j*.013,1.32-j*.031),(s*.14+dx,.20+j*.012,1.19-j*.034),(s*.184+dx,.204+j*.012,1.14-j*.034)]
        hair_lock('Layered_hair_'+side+str(j),points,[.039,.052,.047,.046,.026,.001],hw)
    hair_lock('Face_framing_lock_'+side,[(s*.087,.004,1.77),(s*.118,-.004,1.66),(s*.13,-.015,1.56),(s*.171,-.003,1.502)],[.027,.034,.023,.001],hw)

bone('ik_foot_root',(0,0,0),(0,0,.12),'root',False)
bone('ik_hand_root',(0,0,0),(0,0,.12),'root',False)
bone('ik_hand_gun',BONES['hand_r'][0],BONES['hand_r'][1],'ik_hand_root',False)
for side in ['l','r']:
    bone('ik_foot_'+side,*BONES['foot_'+side][:2],'ik_foot_root',False)
    bone('ik_hand_'+side,*BONES['hand_'+side][:2],'ik_hand_gun',False)

arm=bpy.data.armatures.new('SK_AlchemistMage')
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
character=bpy.context.object;character.name='SKM_AlchemistMage';character.data.name='AlchemistMage_LowPoly'
bm=bmesh.new();bm.from_mesh(character.data);bmesh.ops.triangulate(bm,faces=list(bm.faces));bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(character.data);bm.free()
mod=character.modifiers.new('Skeleton','ARMATURE');mod.object=rig;character.parent=rig
character['reference']='Alchemist mage from the user supplied reference; simplified detail and original low-poly geometry.'
rig['retarget']='Own skeleton, UE5 Manny core names, use IK Retargeter'

def select_model():
    bpy.ops.object.select_all(action='DESELECT');character.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig

select_model()
bpy.ops.export_scene.fbx(filepath=str(OUT/'SKM_AlchemistMage_APose.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',use_tspace=True,add_leaf_bones=False,use_armature_deform_only=False,armature_nodetype='NULL',primary_bone_axis='Y',secondary_bone_axis='X',bake_anim=False,path_mode='COPY',embed_textures=True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'SKM_AlchemistMage_APose.glb'),export_format='GLB',use_selection=True,export_yup=True,export_skins=True,export_animations=False,export_materials='EXPORT')
stats={'name':'The AlchemistMage','vertices':len(character.data.vertices),'triangles':len(character.data.polygons),'bones':len(BONES),'height_cm':round(character.dimensions.z,2),'width_cm':round(character.dimensions.x,2),'material_slots':len(character.material_slots),'unweighted_vertices':sum(not v.groups for v in character.data.vertices),'max_influences':max(len(v.groups) for v in character.data.vertices),'bones_hierarchy':{k:v[2] for k,v in BONES.items()},'upperarm_cm':round((BONES['upperarm_l'][1]-BONES['upperarm_l'][0]).length*S,3),'forearm_cm':round((BONES['lowerarm_l'][1]-BONES['lowerarm_l'][0]).length*S,3),'bind_pose':'Neutral A pose; separate walk, run, cast actions'}
assert stats['unweighted_vertices']==0 and stats['max_influences']<=4
assert [b.name for b in arm.bones if not b.parent]==['root']
(OUT/'model_report.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
print('MODEL_REPORT',json.dumps({k:v for k,v in stats.items() if k!='bones_hierarchy'}),flush=True)

# Neutral studio for shape review: three viewpoints, the same single A pose.
scene.render.engine='CYCLES';scene.cycles.samples=40;scene.cycles.use_denoising=True
scene.render.resolution_x=1200;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
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
data.type='ORTHO';data.ortho_scale=222;data.clip_end=10000
camera.location=(280,-700,225);point_at(camera,(0,0,96))
select_model()
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_distance=360
            area.spaces.active.region_3d.view_location=(0,0,96)
            area.spaces.active.clip_end=10000
scene.render.filepath=str(OUT/'Preview_ThreeQuarter.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'AlchemistMage_APose.blend'))
bpy.ops.render.render(write_still=True)
camera.location=(0,-800,96);point_at(camera,(0,0,96));scene.render.filepath=str(OUT/'Preview_Front.png');bpy.ops.render.render(write_still=True)
camera.location=(280,700,225);point_at(camera,(0,0,96));scene.render.filepath=str(OUT/'Preview_Back.png');bpy.ops.render.render(write_still=True)
print('BUILD_COMPLETE',str(OUT),flush=True)

