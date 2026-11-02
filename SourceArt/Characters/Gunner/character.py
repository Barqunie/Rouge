# An athletic torso with an exposed waist; the vest is one tailored shell.
zbody('Bare_midriff',[(1.075,.142,.084,0),(1.13,.123,.079,0),(1.205,.114,.074,0),(1.265,.125,.080,0),(1.31,.138,.086,0)],'Skin',body_w,16,2)
# Skin hidden underneath the closed vest is omitted to prevent cloth intersections.
tube('Neck',[((0,0,1.49),.048,.042),((0,.004,1.59),.043,.039),((0,0,1.625),.045,.040)],'Skin',body_w,12,2)
def vest_shape(p,a,t):
    front=max(0,math.cos(a))**4
    p.y-=front*.015*(gauss(p.x,-.063,.050)+gauss(p.x,.063,.050))*gauss(p.z,1.39,.048)
    return p
zbody('Cropped_tactical_vest',[(1.281,.130,.088,-.003),(1.31,.141,.099,-.005),(1.365,.158,.109,-.007),(1.414,.158,.102,-.003),(1.446,.167,.087,0),(1.487,.149,.075,0),(1.518,.065,.050,0),(1.55,.050,.046,0),(1.57,.049,.045,0)],'ClothDark',body_w,20,2,vest_shape)
zbody('Vest_hem',[(1.277,.131,.090,-.003),(1.298,.138,.094,-.004)],'Leather',body_w,16,1)
# Shaped breast seams and front zipper are shallow geometry, not tiny dense tubing.
for s in [-1,1]:
    line('Vest_shaped_seam',[(s*.025,-.112,1.311),(s*.087,-.117,1.326),(s*.136,-.091,1.37)],.0015,'ClothLight',body_w,4,2)
    tube('Harness_shoulder_strap',[((s*.128,-.079,1.421),.015,.005),((s*.145,-.039,1.490),.017,.006),((s*.135,.035,1.493),.017,.006),((s*.115,.089,1.397),.015,.005)],'LeatherEdge',body_w,6,2)
    box('Harness_brass_clip',(s*.134,-.066,1.465),(.025,.012,.035),'Brass',body_w,.002)
    box('Harness_clip_dark',(s*.134,-.074,1.465),(.012,.004,.019),'Leather',body_w,.001)
line('Vest_zipper_track',[(0,-.097,1.289),(0,-.128,1.375),(0,-.107,1.444),(0,-.056,1.517),(0,-.048,1.561)],.003,'MetalLight',body_w,4,2)
for j in range(13):
    z=1.30+j*.013;y=-.101-.024*math.sin((z-1.29)/.22*math.pi)
    box('Zipper_tooth',(.004 if j%2 else -.004,y-.003,z),(.005,.004,.003),'MetalLight',body_w,0)
box('Zip_pull',(0,-.06,1.486),(.010,.005,.021),'MetalLight',body_w,.001)
line('Dogtag_chain',[(-.033,-.045,1.517),(-.052,-.071,1.465),(0,-.122,1.427),(.045,-.075,1.463),(.031,-.046,1.516)],.0015,'MetalLight',body_w,4,2)
box('Dogtag',(.007,-.125,1.416),(.017,.004,.027),'MetalLight',body_w,.002)
# One subtle navel and the abdominal oblique planes.
panel('Navel',[(-.003,-.0805,1.174),(.003,-.0805,1.174),(.002,-.080,1.163),(-.002,-.080,1.163)],'Tattoo',body_w,.0004)
for s in [-1,1]:
    panel('Abdominal_plane',[(s*.08,-.061,1.124),(s*.101,-.053,1.153),(s*.098,-.053,1.187),(s*.092,-.057,1.149)],'SkinLight',body_w,.0004)

for side,s in [('l',1),('r',-1)]:
    shoulder,elbow=BONES['upperarm_'+side][:2];wrist,handend=BONES['hand_'+side][:2]
    armw=chain_weights(['clavicle_'+side,'upperarm_'+side,'lowerarm_'+side,'hand_'+side],.04)
    tube('Bare_arm_'+side,[((s*.145,0,1.477),.043,.043),(shoulder,.055,.048),(shoulder.lerp(elbow,.3),.054,.048),(shoulder.lerp(elbow,.63),.044,.040),(elbow,.033,.033),(elbow.lerp(wrist,.22),.040,.035),(elbow.lerp(wrist,.65),.031,.028),(wrist,.023,.023)],'Skin',armw,12,2)
    # Fabric wraps follow the forearm axis; overlapping narrow bands show the layers.
    axis=(wrist-elbow).normalized();u,v=path_frames([elbow,wrist])[0]
    for j in range(7):
        t=.25+j*.078;p=elbow.lerp(wrist,t);r=.041-(t-.25)*.023
        band('Forearm_wrap',p-axis*.009,p+axis*.011,r,'Wrap',armw,10,depth=r*.90)
        if j%2==0:line('Wrap_diagonal',[p+u*r+v*.002,p+u*r*.35+v*(-r*.84)+axis*.015,p-u*r*.6+v*(-r*.7)+axis*.018],.001,'LeatherEdge',armw,4,1)
    band('Wrist_cuff',wrist-axis*.016,wrist+axis*.007,.029,'Leather',armw,10)
    across=Vector((s*.7071068,0,.7071068))
    tube('Fingerless_glove',[(wrist,.026,.023),(wrist.lerp(handend,.48),.037,.024),(handend,.036,.019)],'Leather','hand_'+side,10,2)
    p=wrist.lerp(handend,.63)+Vector((0,-.027,0))
    panel('Glove_backplate',[p+across*.026-axis*.025,p-across*.026-axis*.025,p-across*.024+axis*.020,p+across*.024+axis*.020],'LeatherEdge','hand_'+side,.005)
    box('Glove_green_marker',p+Vector((0,-.005,0)),(.011,.005,.014),'ToxicDark','hand_'+side,.001)
    for finger in ['index','middle','ring','pinky','thumb']:
        chain=[f'{finger}_{i:02}_{side}' for i in [1,2,3]];pts=[BONES[n][0] for n in chain]+[BONES[chain[-1]][1]];fw=chain_weights(chain,.007)
        r=.010 if finger!='thumb' else .012
        tube('Bare_finger_'+finger,[(pts[0],r,r*.90),(pts[1],r*.9,r*.86),(pts[2],r*.78,r*.76),(pts[3],r*.47,r*.5)],'Skin',fw,6,1)
        tube('Glove_finger_base',[(pts[0],r*1.13,r),(pts[0].lerp(pts[1],.7),r*1.05,r*.96)],'Leather',fw,6,1)
    # Three large tattoo motifs conform to the outer front upper arm.
    if side=='l':
        axis=(elbow-shoulder).normalized();u=Vector((axis.z,0,-axis.x)).normalized();v=Vector((0,-1,0))
        motifs=[[(0,0),(.016,.023),(.010,.047),(-.004,.041),(.003,.023),(-.008,.014)],
                [(-.014,.035),(-.025,.055),(-.015,.082),(-.008,.067),(-.016,.055)],
                [(.008,.055),(.023,.072),(.019,.108),(.008,.093),(.014,.078),(-.003,.066)]]
        for poly in motifs:
            points=[]
            for a,t in poly:
                r=.052-.055*t;points.append(shoulder+axis*(.035+t)+u*a+v*math.sqrt(max(.0001,r*r-a*a)))
            panel('Upper_arm_tribal_tattoo',points,'Tattoo','upperarm_l',.0004)

    hip,knee=BONES['thigh_'+side][:2];ankle,ball=BONES['foot_'+side][:2];lw=chain_weights(['thigh_'+side,'calf_'+side,'foot_'+side],.045)
    # Real opening in the knee fabric; there is bare skin behind the torn edge.
    tube('Knee_skin',[(hip.lerp(knee,.55),.074,.071),(knee,.053,.054),(knee.lerp(ankle,.3),.056,.049)],'Skin',lw,10,1)
    centers=[hip+Vector((0,0,.035)),hip.lerp(knee,.18),hip.lerp(knee,.43),hip.lerp(knee,.62),hip.lerp(knee,.72),hip.lerp(knee,.86),knee,knee.lerp(ankle,.16),knee.lerp(ankle,.32),knee.lerp(ankle,.39)]
    radii=[(.078,.079),(.103,.105),(.106,.109),(.100,.103),(.096,.101),(.087,.086),(.087,.088),(.099,.087),(.083,.074),(.065,.061)]
    vs=[];fs=[];N=12
    for i,(c,(rx,ry)) in enumerate(zip(centers,radii)):
        for j in range(N):
            a=j*math.tau/N;fold=1+(.10 if i in [3,7,8] else .035)*math.sin(a*5+i*.8)
            p=c+Vector((math.sin(a)*rx*fold,-math.cos(a)*ry*fold,0))
            if i in [4,5] and j in [0,1,2,11]:
                p.z+=({11:-.018,0:.014,1:.030,2:.017}[j]*(1 if i==4 else .6))*(1 if side=='l' else -1)
            vs.append(p)
    for i in range(len(centers)-1):
        for j in range(N):
            if i==4 and j in ([11,0,1] if side=='l' else [11,0]):continue
            fs.append((i*N+j,i*N+(j+1)%N,(i+1)*N+(j+1)%N,(i+1)*N+j))
    fs.append(tuple(reversed(range(N))));fs.append(tuple((len(centers)-1)*N+j for j in range(N)))
    trouser=mesh('Torn_cargo_pants_'+side,vs,fs,'Cloth',lw)
    trouser.data.materials.append(MATS['ClothLight']);trouser.data.materials.append(MATS['ClothDark'])
    for p in trouser.data.polygons:
        if p.index%9==1:p.material_index=1
        elif p.index%11==4:p.material_index=2
    for row in [4,5]:
        points=[vs[row*N+j] for j in [11,0,1,2]]
        line('Frayed_knee_edge',points,.0025,'ClothLight',lw,4,1)
    kneeplate=knee+Vector((0,-.098,.008))
    panel('Knee_protector',[kneeplate+Vector((-.044,0,.051)),kneeplate+Vector((.044,0,.051)),kneeplate+Vector((.050,-.007,-.027)),kneeplate+Vector((.030,-.012,-.053)),kneeplate+Vector((-.033,-.012,-.053)),kneeplate+Vector((-.050,-.007,-.026))],'LeatherEdge','calf_'+side,.010)
    # Cargo flap sits on the outside front of the thigh.
    pc=hip.lerp(knee,.43)+Vector((s*.077,-.047,-.005))
    box('Cargo_pocket',pc,(.080,.044,.131),'ClothDark','thigh_'+side,.008)
    box('Cargo_pocket_flap',pc+Vector((0,-.027,.042)),(.088,.012,.047),'ClothLight','thigh_'+side,.003)
    box('Cargo_pocket_snap',pc+Vector((0,-.036,.035)),(.013,.004,.013),'Metal','thigh_'+side,.001)
    cuff=knee.lerp(ankle,.37)
    band('Rolled_pants_cuff',cuff+Vector((0,0,.025)),cuff-Vector((0,0,.019)),.073,'ClothLight',lw,10,depth=.066)
    tube('Shin_skin',[(knee.lerp(ankle,.38),.051,.046),(ankle,.034,.034)],'Skin',lw,10,1)
    for j in range(6):
        t=.43+j*.055;c=knee.lerp(ankle,t);r=.052-(t-.43)*.037
        band('Shin_wrap',c+Vector((0,0,.014)),c-Vector((0,0,.012)),r,'Wrap',lw,10,depth=r*.91)
        line('Shin_wrap_seam',[(c.x-r*.80,-r*.55+c.y,c.z+.01),(c.x,-r+c.y,c.z),(c.x+r*.8,-r*.55+c.y,c.z-.013)],.001,'LeatherEdge',lw,4,1)
    fw=chain_weights(['foot_'+side,'ball_'+side],.025)
    tube('Combat_boot_shaft',[(ankle+Vector((0,0,.13)),.049,.048),(ankle+Vector((0,0,.06)),.048,.049),(ankle,.043,.048)],'Leather',lw,10,1)
    tube('Combat_boot_foot',[((s*.151,.048,.081),.046,.068),((s*.151,-.025,.088),.051,.067),((s*.151,-.107,.069),.062,.047),((s*.151,-.210,.051),.064,.035),((s*.151,-.239,.045),.048,.029)],'Leather',fw,10,1)
    box('Boot_sole',(s*.151,-.09,.019),(.129,.302,.033),'Shadow',fw,.012)
    tube('Boot_toe_cap',[((s*.151,-.180,.061),.066,.030),((s*.151,-.225,.053),.056,.027)],'LeatherEdge',fw,8,1)
    panel('Boot_lime_toe',[(s*.151-.039,-.242,.027),(s*.151+.039,-.242,.027),(s*.151+.028,-.238,.051),(s*.151-.026,-.238,.056)],'ToxicDark',fw,.002)
    for j in range(4):
        z=.085+j*.022;y=-.137+j*.023
        for sign in [-1,1]:
            line('Boot_laces',[(s*.151+sign*.026,y,z),(s*.151-sign*.024,y+.014,z+.013)],.002,'Wrap',fw,4,1)
    band('Boot_ankle_strap',(s*.151,0,.227),(s*.151,0,.257),.058,'LeatherEdge',lw,10,depth=.057)
    box('Boot_buckle',(s*.151,-.059,.242),(.036,.012,.040),'MetalLight',lw,.003)
    box('Boot_buckle_hole',(s*.151,-.067,.242),(.022,.004,.024),'Leather',lw,.001)

# Trouser waist joins the two legs below the cropped vest.
zbody('Pants_waist',[(.968,.155,.099,0),(1.038,.165,.104,0),(1.094,.147,.094,0)],'Cloth','pelvis',16,2)
zbody('Waist_belt',[(1.070,.157,.107,0),(1.111,.148,.099,0)],'Leather','pelvis',16,1)
box('Belt_buckle',(-.015,-.111,1.09),(.060,.012,.042),'MetalLight','pelvis',.003)
box('Belt_buckle_hole',(-.015,-.119,1.09),(.041,.004,.026),'Leather','pelvis',.001)
box('Belt_buckle_prong',(-.013,-.124,1.09),(.005,.003,.028),'MetalLight','pelvis',0)
for x in [-.12,-.083,.061,.104]:box('Belt_loop',(x,-.097,1.092),(.014,.014,.054),'LeatherEdge','pelvis',.001)
line('Pants_fly',[(.012,-.109,1.065),(.014,-.112,1.006),(.023,-.105,.969)],.0015,'ClothDark','pelvis',4,1)
for s in [-1,1]:
    for j in range(2 if s==1 else 1):
        c=Vector((s*(.147+j*.039),-.065+j*.022,1.027-j*.009))
        band('Belt_chemical_vial',c-Vector((0,0,.043)),c+Vector((0,0,.034)),.020,'Toxic','pelvis',8)
        for z in [-.049,.040]:band('Vial_endcap',c+Vector((0,0,z-.009)),c+Vector((0,0,z+.009)),.024,'Metal','pelvis',8)
        box('Vial_harness',c+Vector((0,-.018,0)),(.012,.009,.070),'Leather','pelvis',.001)
# Low-poly chain loops sag across the right hip.
for j in range(13):
    t=j/12;p=Vector((-.055-.117*t,-.115+.040*t,1.069-.075*math.sin(math.pi*t)))
    hoop('Belt_chain_link',p,(0,1,0) if j%2 else (.5,1,.6),.008,.0018,'MetalLight','pelvis',6)

# Rigid backpack follows the chest. The lime liquid remains plainly visible.
box('Backpack_frame',(0,.129,1.384),(.180,.061,.258),'Leather','spine_04',.012)
box('Backpack_rear_plate',(0,.167,1.38),(.148,.026,.217),'Metal','spine_04',.006)
band('Backpack_lime_reservoir',(0,.193,1.277),(0,.193,1.486),.069,'Toxic','spine_04',10)
for z in [1.267,1.497]:band('Tank_armored_cap',(0,.193,z-.025),(0,.193,z+.025),.081,'Metal','spine_04',10)
band('Tank_top_valve',(0,.193,1.515),(0,.193,1.553),.026,'MetalLight','spine_04',8)
for x in [-.059,.059]:box('Tank_frame_rail',(x,.245,1.382),(.019,.020,.206),'Leather','spine_04',.003)
box('Tank_label',(0,.264,1.382),(.085,.008,.095),'ClothDark','spine_04',.004)
hazard('Tank_hazard',(0,.270,1.382),.063,'Toxic','spine_04',True)
for s in [-1,1]:
    points=[(s*.055,.183,1.49),(s*.117,.153,1.535),(s*.180,.104,1.49),(s*.192,.078,1.398),(s*.160,.052,1.345)]
    line('Chemical_backpack_hose',points,.016,'ToxicDark','spine_04',8,3)
    line('Hose_lime_stripe',[(x,y-.009,z+.006) for x,y,z in points],.006,'Toxic','spine_04',5,3)
    for point in [points[0],points[-1]]:ellipsoid('Hose_coupling',point,(.022,.022,.026),'Metal','spine_04',8,4)
print('GUNNER_BODY_READY',len(PARTS),flush=True)
