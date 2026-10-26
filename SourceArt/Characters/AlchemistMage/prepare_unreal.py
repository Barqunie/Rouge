from pathlib import Path
O=Path(__file__).resolve().parent
s=(O.parent/'Bulk/setup_unreal.py').read_text(encoding='utf-8')
s=s.replace('Bulk','AlchemistMage').replace('bulk','mage').replace('BK_','AM_').replace('BULK_REPORT','MAGE_REPORT')
s=s.replace('for fraction in [0,.25,.5,.75]:','for frame in range(round(anim.get_play_length()*30)+1):\n                fraction=frame/max(1,round(anim.get_play_length()*30))')
s=s.replace("'sampled_times':4","'sampled_times':len(samples)")
s=s.replace("'reference':reference,'samples':samples","'reference':reference,'samples':samples,'duration':anim.get_play_length(),'fps':30")
(O/'setup_unreal.py').write_text(s,encoding='utf-8')
