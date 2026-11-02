def chain_weights(chain,blend=.05):
    def fn(p):
        best=(1e10,None,0)
        for n in chain:
            a,b,_,_=BONES[n];d=b-a;t=max(0,min(1,(p-a).dot(d)/d.length_squared));dist=(p-a-d*t).length
            if dist<best[0]:best=(dist,n,t)
        _,n,t=best;i=chain.index(n);length=(BONES[n][1]-BONES[n][0]).length
        if i and t*length<blend:
            w=.5+.5*t*length/blend;return {n:w,chain[i-1]:1-w}
        if i<len(chain)-1 and (1-t)*length<blend:
            w=.5+.5*(1-t)*length/blend;return {n:w,chain[i+1]:1-w}
        return {n:1.}
    return fn
body_w=chain_weights(['pelvis']+[f'spine_{i:02}' for i in range(1,6)]+['neck_01','neck_02'],.045)
def bind(obj,weights):
    groups={}
    for v in obj.data.vertices:
        w=weights(v.co/S) if callable(weights) else {weights:1.}
        w={n:x for n,x in sorted(w.items(),key=lambda a:-a[1])[:4] if x>1e-7};total=sum(w.values())
        for n,x in w.items():
            if n not in groups:groups[n]=obj.vertex_groups.get(n) or obj.vertex_groups.new(name=n)
            groups[n].add([v.index],x/total,'REPLACE')
def mesh(name,vs,fs,mat,weights='head',uvs=None,smooth=False):
    data=bpy.data.meshes.new(name);data.from_pydata([Vector(v)*S for v in vs],[],fs);data.update()
    bm=bmesh.new();bm.from_mesh(data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(data);bm.free()
    ob=bpy.data.objects.new(name,data);scene.collection.objects.link(ob);data.materials.append(MATS[mat])
    uv=data.uv_layers.new(name='UVMap')
    for poly in data.polygons:
        poly.use_smooth=smooth
        for li in poly.loop_indices:
            vi=data.loops[li].vertex_index;p=data.vertices[vi].co
            uv.data[li].uv=uvs[vi] if uvs else (p.x*.012+.5,p.z*.005)
    bind(ob,weights);PARTS.append(ob);return ob
def sample_curve(points,steps=6):
    pts=[np.asarray(p,dtype=float) for p in points];out=[]
    for i in range(len(pts)-1):
        p0=pts[max(0,i-1)];p1=pts[i];p2=pts[i+1];p3=pts[min(len(pts)-1,i+2)]
        for k in range(steps):
            t=k/steps
            out.append((p1*2+(-p0+p2)*t+(p0*2-p1*5+p2*4-p3)*t*t+(-p0+p1*3-p2*3+p3)*t*t*t)*.5)
    return out+[pts[-1]]
def path_frames(sampled):
    frames=[];previous=None
    for i,row in enumerate(sampled):
        t=Vector(sampled[min(i+1,len(sampled)-1)][:3])-Vector(sampled[max(i-1,0)][:3]);t.normalize()
        if previous is None:
            ref=Vector((0,-1,0)) if abs(t.y)<.94 else Vector((0,0,1))
            u=t.cross(ref).normalized()
        else:
            u=previous-t*previous.dot(t)
            if u.length<1e-5:u=t.cross(Vector((0,0,1)))
            u.normalize()
        v=u.cross(t).normalized();previous=u;frames.append((u,v))
    return frames
def tube(name,rings,mat,weights='head',sides=10,steps=2,fold=0,cap=True,grooves=0):
    sampled=sample_curve([list(p)+[rw,rd] for p,rw,rd in rings],steps)
    vs=[];fs=[];uv=[];frames=path_frames(sampled)
    for i,row in enumerate(sampled):
        c=Vector(row[:3]);u,v=frames[i];f=i/(len(sampled)-1)
        for j in range(sides):
            a=j*math.tau/sides;r=1+grooves*math.cos(5*a)+fold*math.sin(math.pi*f)**.7*(math.cos(a*7+f*6)+.28*math.sin(f*33+a*2))
            p=c+u*(math.cos(a)*max(.0002,row[3])*r)+v*(math.sin(a)*max(.0002,row[4])*r)
            vs.append(p);uv.append((j/sides,f))
        if i:
            for j in range(sides):fs.append(((i-1)*sides+j,(i-1)*sides+(j+1)%sides,i*sides+(j+1)%sides,i*sides+j))
    if cap:fs+=[tuple(reversed(range(sides))),tuple((len(sampled)-1)*sides+j for j in range(sides))]
    return mesh(name,vs,fs,mat,weights,uv)
def line(name,points,radius,mat,weights='head',sides=8,steps=4):
    return tube(name,[(p,radius,radius) for p in points],mat,weights,sides,steps)
def ellipsoid(name,c,r,mat,weight='head',segments=12,rings=8):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=rings,radius=1)
    ob=bpy.context.object;ob.name=name
    for v in ob.data.vertices:v.co=Vector(tuple((c[i]+v.co[i]*r[i])*S for i in range(3)))
    ob.data.materials.clear();ob.data.materials.append(MATS[mat])
    for p in ob.data.polygons:p.use_smooth=False
    bind(ob,weight);PARTS.append(ob);return ob
def box(name,c,size,mat,weight='pelvis',bevel=.003):
    bpy.ops.mesh.primitive_cube_add(size=1)
    ob=bpy.context.object;ob.name=name;ob.dimensions=Vector(size)*S;ob.location=Vector(c)*S
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    if bevel:
        mod=ob.modifiers.new('Soft tailored edges','BEVEL');mod.width=bevel*S;mod.segments=1
        bpy.ops.object.modifier_apply(modifier=mod.name)
    ob.data.materials.append(MATS[mat])
    for p in ob.data.polygons:p.use_smooth=False
    bind(ob,weight);PARTS.append(ob);return ob
def zbody(name,profile,mat,weights,sides=16,steps=2,shape=None,open_top=False):
    rows=sample_curve(profile,steps);vs=[];fs=[];uv=[]
    for i,(z,rx,ry,cy) in enumerate(rows):
        for j in range(sides):
            a=j*math.tau/sides;p=Vector((math.sin(a)*rx,cy-math.cos(a)*ry,z))
            if shape:p=shape(p,a,i/(len(rows)-1))
            vs.append(p);uv.append((j/sides,i/(len(rows)-1)))
        if i:
            for j in range(sides):fs.append(((i-1)*sides+j,(i-1)*sides+(j+1)%sides,i*sides+(j+1)%sides,i*sides+j))
    fs.append(tuple(reversed(range(sides))))
    if not open_top:fs.append(tuple((len(rows)-1)*sides+j for j in range(sides)))
    return mesh(name,vs,fs,mat,weights,uv)
def gauss(x,center,width):return math.exp(-((x-center)/width)**2)

