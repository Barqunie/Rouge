
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

