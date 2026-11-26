from functools import partial
def run_command(cmd,log,*,cwd):
 p=subprocess.run(cmd,cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
def replace_once(text, old, new, label):
 count=text.count(old)
 if count != 1:
  raise ValueError(f'{label}: expected one source anchor, found {count}')
 return text.replace(old,new,1)

 ap=argparse.ArgumentParser()
 ap.add_argument('--out',type=Path)
 ap.add_argument('--source-root',type=Path,default=ROOT,help='support checkout with the oracle, media and build dependencies')
 ap.add_argument('--runtime',type=Path,help='production runtime; adjacent source/header overrides are used by every build')
 ap.add_argument('--modes',nargs='+',default=['O0','O2','UBSan'])
 a=ap.parse_args()
 root=a.source_root.resolve(strict=True)
 run=partial(run_command,cwd=root)
 runtime_path=(a.runtime or root/'pc_port/src/battle_mips_runtime.c').resolve(strict=True)
 test_path=Path(__file__).resolve().with_name('battle_file1_adoption_test.c')
 def selected_source(name):
  adjacent=runtime_path.parent/name
  return adjacent if adjacent.is_file() else root/'pc_port/src'/name
 adapter_path=selected_source('battle_file1_controller.inc')
 impl_rel=Path('../../src/battle_command_file1/message_controller_impl.inc')
 impl_path=(adapter_path.parent/impl_rel).resolve()
 if not impl_path.is_file():
  impl_path=root/'src/battle_command_file1/message_controller_impl.inc'
 cpu_path=selected_source('battle_mips_adapter.c')
 compat_path=selected_source('psyq_compat.c')
 oracle_path=root/'pc_port/tests/battle_command_file1_controller_retail_test.c'
 src=oracle_path.read_text()
 payload=root/'disc/battle_command_file1.bin'
 paths=[runtime_path,adapter_path,selected_source('battle_file1_controller.h'),selected_source('battle_mips_runtime_internal.h'),selected_source('battle_mips_adapter.h'),cpu_path,compat_path,impl_path,impl_path.with_name('message_controller_bindings.inc'),impl_path.with_name('xeno_battle_command_file1_controller.h'),test_path,Path(__file__),oracle_path]
 elf=['--elf','build/out/slus_006.64.elf'] if (root/'build/out/slus_006.64.elf').exists() else []
 common=['-std=gnu17','-fno-pie','-DXENO_PC_PORT','-DSKIP_ASM','-D_LANGUAGE_C','-DUSE_EXTENDED_PRIM_POINTERS=0','-DAUDIT_YIELDS=3','-include','assert.h','-ffunction-sections','-fdata-sections','-iquote',str(runtime_path.parent),'-Ipc_port/include_shim','-Iinclude','-Ipc_port/src',f'-I{out}','-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','-Isrc/battle_command_file1']
  for name,source,warnings in [('compat',compat_path,['-w']),('test',test_path,['-Wall','-Wextra','-Werror']),('cpu',cpu_path,['-Wall','-Wextra','-Werror'])]:
   run(['clang',*common,*flags,*warnings,f'-DBATTLE_RUNTIME_SOURCE="{runtime_path}"','-c',source,'-o',str(out/f'{mode}.{name}.o')],out/f'{mode}.{name}.build.log')
 adapter=adapter_path.read_text()
 runtime=runtime_path.read_text()
  'wrong-archive':('cpu->gpr[4] == 0x20 && cpu->gpr[5] == 0))','cpu->gpr[5] == 0))'),
  'disc1-only':('return PcPort_BattleArchiveDisc() == (archive == 3087u ? 1 : 2);','return archive == 3087u && PcPort_BattleArchiveDisc() == 1;'),
  'ignore-disc-base-pair':('return PcPort_BattleArchiveDisc() == (archive == 3087u ? 1 : 2);','int disc = PcPort_BattleArchiveDisc(); return disc == 1 || disc == 2;'),
  'accept-unknown-medium':('return PcPort_BattleArchiveDisc() == (archive == 3087u ? 1 : 2);','int disc = PcPort_BattleArchiveDisc(); return !disc || disc == (archive == 3087u ? 1 : 2);'),
  'trust-selector-result-mismatch':('(target != 0x80028470u || result == archive) &&',''),
  'allow-wrong-subdirectory':('cpu->gpr[4] == 0x20 && cpu->gpr[5] == 0))','cpu->gpr[4] == 0x20))'),
  'allow-wrong-wrapper':('if ((target == 0x8008ab4cu ||','if ((target == 0x8008ab4cu || target == 0x8008ab70u || target == 0x8008ab94u ||'),
  'skip-postread-disc-check':('archive == (uint32_t)candidate && file1_mounted_archive_matches(archive)) {','archive == (uint32_t)candidate) {'),
  'skip-postread-base-check':('archive == (uint32_t)candidate && file1_mounted_archive_matches(archive)) {','file1_mounted_archive_matches((uint32_t)candidate)) {'),
 expected_kills={
  'wrong-archive':'wrong directory forged valid base',
  'disc1-only':'disc 2 valid direct selector',
  'ignore-disc-base-pair':'disc 2 header with disc 1 media',
  'accept-unknown-medium':'absent or unauthenticated medium',
  'trust-selector-result-mismatch':'wrong selector result on disc 1',
  'allow-wrong-subdirectory':'wrong subdirectory forged valid base',
  'allow-wrong-wrapper':'wrapper subdirectory 2 forged valid on disc 1',
  'skip-postread-disc-check':'disc=1 fault=3',
  'skip-postread-base-check':'disc=1 fault=4',
 }
 # Never reuse objects left by another invocation when only non-O2 modes were requested.
 if 'O2' not in a.modes:
  for dep,source in [('compat',compat_path),('cpu',cpu_path)]:
   run(['clang',*common,'-O2','-w','-c',source,'-o',str(out/f'O2.{dep}.o')],out/f'O2.{dep}.build.log')
  mutant=replace_once(adapter,old,new,name)
  mutant=replace_once(mutant,f'#include "{impl_rel}"',f'#include "{impl_path}"',name+' implementation')
  rs=replace_once(runtime,'#include "battle_file1_controller.inc"',f'#include "{target}"',name+' runtime')
  run(['clang',*common,'-O2','-Wall','-Wextra','-Werror',f'-DBATTLE_RUNTIME_SOURCE="{rpath}"','-c',test_path,'-o',str(out/(name+'.o'))],out/(name+'.build.log'))
  result=subprocess.run([str(out/name),str(payload)],cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
  if name in expected_kills:
   assert expected_kills[name] in result.stdout,(name,expected_kills[name],result.stdout[-2000:])
   manifest[name]['observed_rejection']=expected_kills[name]
