
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


