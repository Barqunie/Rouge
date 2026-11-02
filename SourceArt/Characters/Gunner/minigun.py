# Weapon local axes: muzzle points -Y, Z up. Grip datum is at the origin.
# Kept as a separate prop: no gun geometry is joined into the character mesh.
box('Receiver',(0,-.12,.025),(.235,.365,.185),'Metal','root',.018)
box('Receiver_top_cover',(0,-.14,.129),(.198,.260,.047),'ClothDark','root',.010)
box('Rear_receiver_cap',(0,.072,.019),(.213,.054,.170),'Leather','root',.012)
band('Front_motor_housing',(0,-.304,.025),(0,-.412,.025),.139,'Metal','root',12)
band('Motor_dark_recess',(0,-.409,.025),(0,-.434,.025),.119,'Shadow','root',12)
band('Motor_outer_rim',(0,-.398,.025),(0,-.423,.025),.144,'MetalLight','root',12)
for s in [-1,1]:
    # Canister and armoured cradle on either side of the action.
    c=Vector((s*.163,-.12,-.022))
    band('Weapon_lime_canister',c+Vector((0,.115,0)),c-Vector((0,.132,0)),.063,'ToxicDark','root',10)
    for y in [.110,-.129]:band('Canister_cap',c+Vector((0,y+.017,0)),c+Vector((0,y-.017,0)),.074,'MetalLight','root',10)
    for y in [.045,-.065]:band('Canister_retaining_strap',c+Vector((0,y+.012,0)),c+Vector((0,y-.012,0)),.073,'Leather','root',10)
    box('Receiver_side_plate',(s*.123,-.116,.036),(.023,.153,.095),'MetalLight','root',.006)
    for y in [-.174,-.06]:
        for z in [.006,.064]:ellipsoid('Receiver_bolt',(s*.138,y,z),(.006,.008,.008),'Metal','root',6,4)
    box('Canister_hazard_plate',(s*.238,-.12,-.022),(.008,.103,.090),'ClothDark','root',.003)
    first=len(PARTS);hazard('Canister_hazard',(0,0,0),.068,'Toxic','root')
    for ob in PARTS[first:]:
        for vert in ob.data.vertices:
            p=vert.co.copy();vert.co=Vector((s*24.3+s*p.y,-12+p.x,-2.2+p.z))
# Top carry handle: open space under the handle is real empty geometry.
tube('Carry_handle_frame',[((0,.026,.12),.025,.022),((0,.041,.246),.027,.022),((0,-.010,.284),.026,.022),((0,-.249,.284),.026,.022),((0,-.285,.244),.026,.022),((0,-.288,.12),.026,.022)],'Leather','root',6,1)
band('Wrapped_carry_grip',(0,-.038,.284),(0,-.225,.284),.030,'Wrap','root',8)
for j in range(7):hoop('Grip_wrap_seam',(0,-.05-j*.025,.284),(0,1,0),.030,.0015,'LeatherEdge','root',8)
for y in [.015,-.275]:box('Carry_handle_mount',(0,y,.138),(.079,.065,.031),'MetalLight','root',.004)
tube('Rear_trigger_grip',[((0,.063,.045),.030,.024),((0,.136,-.035),.029,.025),((0,.128,-.131),.025,.023)],'Leather','root',8,1)
line('Trigger_guard',[(0,.066,.025),(0,.192,-.012),(0,.189,-.108),(0,.129,-.133)],.009,'Metal','root',6,1)
box('Trigger',(0,.127,-.018),(.012,.013,.042),'MetalLight','root',.002)
line('Rear_chemical_hose',[(.087,.035,.094),(.152,.107,.091),(.213,.101,.014),(.185,.045,-.04)],.012,'ToxicDark','root',8,2)
for s in [-1,1]:
    line('Receiver_lime_conduit',[(s*.073,-.039,.153),(s*.073,-.248,.153)],.011,'Toxic','root',6,1)
    for y in [-.04,-.24]:band('Conduit_connector',(s*.073,y+.010,.153),(s*.073,y-.010,.153),.016,'Metal','root',8)
box('Hazard_front_plate',(0,-.307,.085),(.086,.010,.075),'ClothDark','root',.004)
hazard('Receiver_hazard',(0,-.314,.084),.057,'Toxic','root')
RECEIVER_PARTS=list(PARTS);PARTS=[]

# Six hollow barrels surround a central drive shaft. Their muzzle openings are modeled.
band('Central_rotor_shaft',(0,-.412,.025),(0,-.966,.025),.027,'Metal','root',10)
for i in range(6):
    a=i*math.tau/6;x=math.sin(a)*.087;z=.025+math.cos(a)*.087
    band('Barrel_outer_'+str(i),(x,-.424,z),(x,-.986,z),.028,'Metal','root',10)
    band('Barrel_breech_band',(x,-.437,z),(x,-.470,z),.033,'MetalLight','root',10)
    # Glow is limited to small windows between the structural barrel sections.
    band('Barrel_chemical_window',(x,-.498,z),(x,-.611,z),.033,'Toxic','root',10)
    band('Barrel_dark_tip',(x,-.962,z),(x,-1.003,z),.034,'Shadow','root',10)
    # A recessed black bore and open annular rim provide real visible muzzle depth.
    band('Recessed_bore',(x,-1.002,z),(x,-1.007,z),.021,'Shadow','root',10)
    hoop('Muzzle_annular_lip',(x,-1.018,z),(0,1,0),.027,.007,'MetalLight','root',10)
for y,w in [(-.675,.049),(-.888,.043),(-.980,.042)]:
    # Open radial collar, not a solid plate blocking the barrel holes.
    hoop('Barrel_cluster_collar',(0,y,.025),(0,1,0),.119,.019,'MetalLight','root',12)
    for i in range(6):
        a=i*math.tau/6
        band('Collar_axial_block',(math.sin(a)*.119,y-w*.5,.025+math.cos(a)*.119),(math.sin(a)*.119,y+w*.5,.025+math.cos(a)*.119),.022,'MetalLight','root',6)
        if y>-.95:ellipsoid('Collar_bolt',(math.sin(a)*.140,y,.025+math.cos(a)*.140),(.006,.008,.006),'Metal','root',6,4)
BARREL_PARTS=list(PARTS)
PARTS=RECEIVER_PARTS+BARREL_PARTS
print('MINIGUN_READY',len(PARTS),flush=True)
