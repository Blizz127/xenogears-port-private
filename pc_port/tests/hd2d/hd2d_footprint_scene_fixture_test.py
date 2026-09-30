#!/usr/bin/env python3
"""Exercise the actual wrapper with stub scene state, no renderer/retail data."""
from pathlib import Path
import tempfile,subprocess
root=Path(__file__).resolve().parents[3]
s=(root/'pc_port/tests/hd2d_footprint_scene_fixture.c').read_text()
s='\n'.join(line for line in s.splitlines() if not line.startswith('#include "'))
pre='''#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <assert.h>
struct Actor { uintptr_t pSpriteData; } actors[3], *g_FieldActors=actors;
int g_FieldNumActors=3,g_PlayerActorIndex=1,g_GameSceneMapNum=1;
unsigned g_PcPortPresentedFrames;
static int active=1,calls;
int PcPort_QuickCheckpointFieldIsActive(void){return active;}
void __real_func_8001E298(void *sprite,void *ot){(void)sprite;(void)ot;++calls;}
'''
post='''
int main(void){
actors[0].pSpriteData=10;actors[1].pSpriteData=20;actors[2].pSpriteData=30;
__wrap_func_8001E298((void*)10,0);assert(calls==1);
setenv("XENO_HD2D_FOOTPRINT_ISOLATE_PLAYER","1",1);
__wrap_func_8001E298((void*)10,0);assert(calls==1);
__wrap_func_8001E298((void*)20,0);assert(calls==2);
actors[0].pSpriteData=20;__wrap_func_8001E298((void*)20,0);assert(calls==3);
__wrap_func_8001E298((void*)40,0);assert(calls==4);
__wrap_func_8001E298(0,0);assert(calls==5);
g_PlayerActorIndex=3;__wrap_func_8001E298((void*)30,0);assert(calls==6);
g_PlayerActorIndex=1;g_FieldNumActors=1537;__wrap_func_8001E298((void*)30,0);assert(calls==7);
g_FieldNumActors=3;active=0;__wrap_func_8001E298((void*)30,0);assert(calls==8);
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td);(p/'test.c').write_text(pre+s+post)
 for flags in [['-O0'],['-O2'],['-O2','-fsanitize=undefined','-fno-sanitize-recover=all']]:
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*flags,str(p/'test.c'),'-o',str(p/'test')],check=True)
  subprocess.run([str(p/'test')],check=True)
print('scene isolation fixture: player/alias preserved, nonplayer omitted, unsafe scope forwarded PASS')
