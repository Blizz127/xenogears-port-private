#!/usr/bin/env python3
"""Replay owner baseline without mutating it; compile actual guard/wrapper text."""
from pathlib import Path
import subprocess,tempfile,re
root=Path(__file__).resolve().parents[3]
base=root/'pc_port/extern/PsyCross'
patch=root/'pc_port/patches/hd2d/zz_producer_enabled.patch'
files=['include/PsyX/PsyX_render.h','src/PsyX_main.cpp','src/gpu/PsyX_GPU.cpp','src/render/PsyX_render.cpp']
with tempfile.TemporaryDirectory() as td:
 p=Path(td)
 # The vendor checkout may already contain every patch; rebuild the exact
 # owner baseline from its pinned commit, never from mutable scratch files.
 archive=subprocess.run(['git','-C',str(base),'archive','e56e4cd'],check=True,stdout=subprocess.PIPE).stdout
 subprocess.run(['tar','-xf','-','-C',td],input=archive,check=True)
 script=(root/'pc_port/build_port.sh').read_text()
 owner=re.findall(r'^\s*apply_psycross_patch \"\$ROOT/(.*?)\" \"[^\"]+\"(?: \"(unidiff-zero)\")?',script,re.M)
 assert len(owner)==59
 for rel,mode in owner:
  command=['git','apply']+(['--unidiff-zero'] if mode else [])+[str(root/rel)]
  subprocess.run(command,cwd=td,check=True,stdout=subprocess.DEVNULL)
 for earlier in sorted(patch.parent.glob('*.patch')):
  if earlier.name>=patch.name:break
  subprocess.run(['git','apply',str(earlier)],cwd=td,check=True,stdout=subprocess.DEVNULL)
 baseline={name:(p/name).read_bytes() for name in files}
 for args in [[],['-R'],[]]:
  subprocess.run(['patch','--batch','-p1','-d',td,*args],input=patch.read_bytes(),check=True,stdout=subprocess.DEVNULL)
  if args:
   for name in files: assert (p/name).read_bytes()==baseline[name]
 header=(p/files[0]).read_text();render=(p/files[3]).read_text();gpu=(p/files[2]).read_text();main=(p/files[1]).read_text()
 assert 'isPsxShader && PcPort_Hd2dFrameBuffers &&' in render
 assert gpu.count('if (PsyX_Hd2dProducerActive())')==5
 assert render.count('if (PsyX_Hd2dProducerActive())')==5 # wrapper plus four GL sites
 assert main.count('else if (PcPort_Hd2dFrameBuffers) PcPort_Hd2dFrameBuffers(NULL);')==2
 helper=re.search(r'static inline int PsyX_Hd2dProducerActive\(void\)\n\{.*?\n\}',header,re.S).group()
 wrapper=re.search(r'extern "C" void PsyX_Hd2dTagSpritePacket\(const void\* packet\)\n\{.*?\n\}',render,re.S).group()
 for mode in ['off','on','absent','nohook']:
  source='#include <cassert>\nint rawCalls, queryCalls;\n'
  source+='extern "C" int PcPort_Hd2dEnabled(void) __attribute__((weak));\n'
  source+='extern "C" void PcPort_Hd2dFrameBuffers(const void*) __attribute__((weak));\n'
  definitions='extern int queryCalls;\n'
  if mode!='nohook':definitions+='extern "C" void PcPort_Hd2dFrameBuffers(const void*) {}\n'
  if mode not in ['absent','nohook']:definitions+='extern "C" int PcPort_Hd2dEnabled(void) {++queryCalls;return '+('0' if mode=='off' else '1')+';}\n'
  source+='static void PsyX_Hd2dTagSpritePacketRaw(const void*) {++rawCalls;}\n'+helper+'\n'+wrapper+'\n'
  source+='int main(){PsyX_Hd2dTagSpritePacket((const void*)1);assert(rawCalls=='+('1' if mode in ['on','absent'] else '0')+');assert(queryCalls=='+('1' if mode in ['off','on'] else '0')+');}\n'
  (p/'test.cpp').write_text(source)
  (p/'defs.cpp').write_text(definitions)
  subprocess.run(['c++','-O2','-Wall','-Wextra','-Werror',str(p/'test.cpp'),str(p/'defs.cpp'),'-o',str(p/'test')],check=True)
  subprocess.run([str(p/'test')],check=True)
 # Compile the actual inserted declaration/helper in both header consumer modes.
 for language,compiler in [('c','cc'),('cpp','c++')]:
  decl='extern void PcPort_Hd2dFrameBuffers(const void*) __attribute__((weak));\n'
  decl+='extern int PcPort_Hd2dEnabled(void) __attribute__((weak));\n'
  unit=decl+helper+'\nint main(void){return PsyX_Hd2dProducerActive();}\n'
  if language=='cpp':unit='extern "C" {\n'+decl+helper+'\n}\nint main(){return PsyX_Hd2dProducerActive();}\n'
  (p/('consumer.'+language)).write_text(unit)
  subprocess.run([compiler,'-Wall','-Wextra','-Werror',str(p/('consumer.'+language)),'-o',str(p/'consumer')],check=True)
  subprocess.run([str(p/'consumer')],check=True)
 # Replay later lane patches against this exact producer baseline too.
 for later in sorted(patch.parent.glob('*.patch')):
  if later.name<=patch.name: continue
  payload=later.read_bytes()
  subprocess.run(['git','apply',str(later)],cwd=td,check=True)
  subprocess.run(['git','apply','--reverse','--check',str(later)],cwd=td,check=True)
  subprocess.run(['git','apply','--reverse',str(later)],cwd=td,check=True)
  subprocess.run(['git','apply',str(later)],cwd=td,check=True)
 for entry in patch.parent.glob('*.patch'):
  marker='/* _hd2d_'+entry.stem+' */'
  assert any(marker in (p/name).read_text() for name in files),entry.name
print('producer enable: full ordered patch replay, selective runtime guards, presence-only shader setup, OFF/ON/absent/nohook PASS')
