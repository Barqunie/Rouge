# Facial planes are deliberately faceted, with sharper sculpted eye sockets.
HP=[(1.606,.028,.034,-.005),(1.629,.051,.049,-.003),(1.655,.068,.059,0),(1.683,.079,.066,.003),(1.717,.083,.070,.005),(1.750,.082,.073,.008),(1.783,.080,.074,.012),(1.815,.064,.061,.012),(1.834,.031,.030,.012)]
def face_shape(p,a,t):
    front=max(0,math.cos(a))**6
    p.y+=front*(.003*(gauss(p.x,-.040,.022)+gauss(p.x,.040,.022))*gauss(p.z,1.724,.017)-.007*gauss(p.x,0,.013)*gauss(p.z,1.715,.026))
    return p
zbody('Faceted_female_face',HP,'Skin','head',32,2,face_shape)
def face_y(x,z):
    rx=np.interp(z,[p[0] for p in HP],[p[1] for p in HP]);ry=np.interp(z,[p[0] for p in HP],[p[2] for p in HP]);cy=np.interp(z,[p[0] for p in HP],[p[3] for p in HP])
    co=math.sqrt(max(.03,1-(x/rx)**2));return cy-co*ry
for s in [-1,1]:
    ellipsoid('Ear',(s*.081,.005,1.699),(.017,.012,.028),'Skin','head',8,6)
    ellipsoid('Ear_inner',(s*.087,-.006,1.698),(.009,.003,.017),'Tattoo','head',8,4)
    hoop('Ear_ring',(s*.087,-.002,1.673),(0,1,0),.006,.0016,'Brass','head',8)
    cx=s*.038;cz=1.728;vs=[];fs=[];top=[];bottom=[]
    for i in range(13):
        u=-1+2*i/12;x=cx+u*.024;tilt=s*u*.0032
        hi=cz+.0087*math.sin(math.pi*(u+1)/2)+tilt;lo=cz-.0053*math.sin(math.pi*(u+1)/2)+tilt
        cy=face_y(x,cz)-.003
        for j in range(4):
            t=j/3;vs.append((x,cy-.0035*(1-u*u)*math.sin(t*math.pi),lo+(hi-lo)*t))
        top.append((x,cy-.001,hi));bottom.append((x,cy-.001,lo))
    for i in range(12):
        for j in range(3):k=i*4+j;fs.append((k,k+4,k+5,k+1))
    mesh('Almond_eye_white',vs,fs,'White','head')
    y=face_y(cx,cz)-.008
    ellipsoid('Amber_iris',(cx,y,cz),(.0065,.002,.007),'Iris','head',12,6)
    ellipsoid('Eye_pupil',(cx,y-.002,cz),(.0034,.0015,.0042),'Shadow','head',10,4)
    ellipsoid('Eye_glint',(cx-.0017,y-.0035,cz+.0028),(.0013,.001,.0015),'White','head',6,4)
    line('Lash_upper',top,.0014,'HairDark','head',4,1)
    line('Lid_lower',bottom,.0013,'SkinLight','head',4,1)
    outer=Vector(top[-1] if s>0 else top[0]);panel('Lash_wing',[outer,outer+Vector((s*.007,.001,.004)),outer-Vector((s*.003,.001,.002))],'HairDark','head',.001)
    brow=[]
    for x,z in [(s*.016,1.750),(s*.039,1.764),(s*.061,1.760),(s*.071,1.751),(s*.057,1.754),(s*.038,1.756),(s*.020,1.744)]:brow.append((x,face_y(x,z)-.003,z))
    panel('Expressive_eyebrow',brow,'HairDark','head',.002)

# Angular respirator, with two recessed lime filter inserts.
mask=[(-.063,-.079,1.686),(-.026,-.091,1.711),(0,-.109,1.706),(.026,-.091,1.711),(.063,-.079,1.686),(.059,-.080,1.645),(.026,-.095,1.621),(0,-.103,1.618),(-.026,-.095,1.621),(-.059,-.080,1.645)]
center=Vector((0,-.127,1.665));vs=[Vector(p) for p in mask]+[center];fs=[(i,(i+1)%10,10) for i in range(10)]
maskob=mesh('Respirator_faceted_shell',vs,fs,'Metal','head');maskob.data.materials.append(MATS['ClothDark']);maskob.data.materials.append(MATS['MetalLight'])
for p in maskob.data.polygons:p.material_index=1 if p.index%3==0 else (2 if p.index in [1,2] else 0)
line('Mask_rubber_seal',mask+[mask[0]],.0035,'Shadow','head',5,1)
panel('Mask_central_plate',[(-.019,-.120,1.692),(.019,-.120,1.692),(.023,-.128,1.651),(0,-.132,1.627),(-.023,-.128,1.651)],'ClothDark','head',.005)
for s in [-1,1]:
    c=Vector((s*.052,-.092,1.661))
    band('Respirator_filter_housing',c+Vector((0,.003,0)),c+Vector((0,-.019,0)),.027,'Leather','head',8)
    band('Respirator_filter_rim',c+Vector((0,-.019,0)),c+Vector((0,-.024,0)),.024,'MetalLight','head',8)
    band('Lime_filter_insert',c+Vector((0,-.024,0)),c+Vector((0,-.026,0)),.0185,'Toxic','head',8)
    for j in [-1,0,1]:box('Filter_grille',(c.x+j*.007,c.y-.028,c.z),(.0025,.002,.024),'ClothDark','head',.0005)
    line('Mask_strap',[(s*.060,-.072,1.681),(s*.087,-.008,1.683),(s*.075,.057,1.687)],.005,'Leather','head',6,2)

# Shoulder-length layered hair with asymmetric waves and sharp tapered ends.
def hw(p):
    side='l' if p.x>=0 else 'r';w=chain_weights([f'hair_{side}_{j:02}' for j in [1,2,3]],.025)(p)
    t=max(0,min(1,(1.77-p.z)/.10));return dict({n:v*t for n,v in w.items()},head=1-t)
def lock(name,pts,widths,mat='Hair',weight=hw):
    ob=tube(name,[(p,w,w*.38) for p,w in zip(pts,widths)],mat,weight,5,2)
    ob.data.materials.append(MATS['HairLight']);ob.data.materials.append(MATS['HairDark'])
    for poly in ob.data.polygons:
        if poly.index%5==1:poly.material_index=1
        elif poly.index%5==4:poly.material_index=2
    return ob
for s in [-1,1]:
    for j in range(3):
        dx=(j-1)*.027;shift=.009*math.sin(j*2+s)
        pts=[(s*.057+dx,.057+j*.017,1.789-j*.004),(s*.085+dx+shift,.091+j*.014,1.710-j*.012),(s*.076+dx-shift,.119+j*.013,1.626-j*.020),(s*.117+dx+shift,.113+j*.015,1.560-j*.014),(s*.108+dx,.139+j*.013,1.503-j*.018),(s*(.135+j*.012)+dx,.137+j*.013,1.474-j*.021)]
        lock('Layered_back_hair',pts,[.021,.031,.034,.030,.018,.0008])
    lock('Face_framing_wave',[(s*.075,-.009,1.787),(s*.094,-.022,1.724),(s*.105,-.017,1.668),(s*.091,-.034,1.613),(s*.132,-.025,1.580)],[.014,.024,.021,.015,.0008])
for j in [-1,0,1]:
    lock('Nape_covering_layer',[(j*.026,.079,1.789),(j*.031+.008,.102,1.714),(j*.035-.008,.121,1.637),(j*.040+.016,.137,1.567),(j*.049-.011,.148,1.509),(j*.052+.015,.151,1.477-abs(j)*.013)],[.022,.037,.038,.031,.020,.0008])
lock('Long_side_fringe',[(.047,-.050,1.798),(.002,-.078,1.787),(-.046,-.085,1.759),(-.082,-.066,1.729),(-.113,-.032,1.727)],[.012,.022,.023,.015,.0008],weight='head')
lock('Upper_swept_fringe',[(.027,-.042,1.804),(-.027,-.070,1.795),(-.075,-.067,1.777),(-.110,-.025,1.779)],[.010,.020,.017,.0008],weight='head')
lock('Short_right_fringe',[(.057,-.047,1.799),(.080,-.051,1.771),(.092,-.025,1.747),(.114,-.012,1.756)],[.012,.016,.012,.0008],weight='head')

# Soft asymmetric beret and goggles resting above the eyebrows.
def cap_shape(p,a,t):
    p.x-=.015*t;p.z+=.010*math.sin(a+.7)*t
    return p
zbody('Beret',[(1.799,.088,.079,.018),(1.813,.101,.087,.018),(1.846,.096,.080,.026),(1.867,.065,.063,.029),(1.877,.020,.025,.029)],'ClothLight','head',12,1,cap_shape)
zbody('Beret_dark_band',[(1.793,.088,.080,.018),(1.808,.091,.082,.018)],'Leather','head',12,1)
line('Goggle_strap',[(-.076,-.033,1.834),(-.096,.025,1.837),(0,.109,1.84),(.090,.023,1.833),(.070,-.035,1.833)],.009,'Leather','head',6,2)
for s in [-1,1]:
    c=Vector((s*.036,-.069,1.840))
    points=[c+Vector((-.030,0,-.016)),c+Vector((.027,0,-.016)),c+Vector((.025,.006,.017)),c+Vector((-.024,.006,.018))]
    panel('Goggle_housing',points,'MetalLight','head',.009)
    panel('Goggle_dark_lens',[c+Vector((-.023,-.002,-.011)),c+Vector((.020,-.002,-.011)),c+Vector((.019,.002,.012)),c+Vector((-.018,.002,.013))],'Shadow','head',.001)
    line('Goggle_glass_glint',[c+Vector((-.017,-.003,.007)),c+Vector((.003,-.003,.010))],.001,'ClothLight','head',4,1)
box('Goggle_bridge',(0,-.074,1.837),(.014,.012,.008),'Metal','head',.001)
print('GUNNER_HEAD_READY',len(PARTS),flush=True)
