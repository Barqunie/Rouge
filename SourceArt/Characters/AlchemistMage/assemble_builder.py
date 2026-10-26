"""Assemble a standalone builder from the project's reusable geometry helpers."""
from pathlib import Path
O=Path(__file__).resolve().parent
bulk=(O.parent/'Bulk/build_bulk.py').read_text(encoding='utf-8')
needle=(O.parent/'Needle/build_needle.py').read_text(encoding='utf-8')
helpers=bulk[:bulk.index('# Broad, compact humanoid skeleton')]
helpers=helpers.replace('Bulk','AlchemistMage').replace('random.seed(37)','random.seed(64)')
helpers=helpers.replace('Reproducible low-poly AlchemistMage chempunk tank','Reproducible low-poly alchemist mage')
a=helpers.index('PALETTE = {');b=helpers.index('\npalette = ',a)
helpers=helpers[:a]+"PALETTE = {'skin':(224,176,150),'ivory':(224,218,198),'leather':(49,41,39),'hair':(118,19,55),'gold':(169,127,64),'shadow':(25,23,25),'toxic':(161,218,19),'lining':(87,110,32)}"+helpers[b:]
arms=needle[needle.index('HAND_FRAMES={}'):needle.index('    hip=Vector')]
arms=arms[:arms.index('    grip=wrist+')]
arms=arms.replace('(s*.235,0,1.485)','(s*.20,0,1.47)').replace('(s*.45,-.018,1.27)','(s*.397,-.018,1.27)').replace('(s*.646,-.025,1.074)','(s*.58,-.025,1.085)')
arms=arms.replace("'cloth'","'leather'").replace("'metal'","'gold'")
arms=arms.replace("'skin',fw,6","'leather',fw,6").replace("'skin',chain_weights(thumb,.01),6","'leather',chain_weights(thumb,.01),6")
arms=arms.replace(".092",".083").replace(".079",".073").replace(".088",".080").replace(".081",".075").replace(".063",".058")
suffix=bulk[bulk.index("arm=bpy.data.armatures.new('SK_Bulk')"):]
suffix=suffix.replace('Bulk','AlchemistMage').replace('The AlchemistMage chempunk tank, user supplied reference sheet; one neutral A-pose model.','Alchemist mage from the user supplied reference; simplified detail and original low-poly geometry.')
suffix=suffix.replace("'A pose only; no concept action poses authored'","'Neutral A pose; separate walk, run, cast actions'")
suffix=suffix.replace("data.ortho_scale=290","data.ortho_scale=222").replace('resolution_x=1400','resolution_x=1200').replace('resolution_y=1400','resolution_y=1200')
suffix=suffix.replace('(330,-700,280)','(280,-700,225)').replace('(330,700,280)','(280,700,225)').replace('(0,0,108)','(0,0,96)').replace('(0,0,109)','(0,0,96)').replace('(0,-800,109)','(0,-800,96)')
(O/'build_mage.py').write_text(helpers+(O/'geometry_body.py').read_text()+arms+(O/'geometry_details.py').read_text()+suffix,encoding='utf-8')
