/* Link storage for unrelated matrix/heap bridge dispatch in this fixture. */
int matrixLevel;
MATRIX stack[20], *currentMatrix = stack;
unsigned g_HeapIsErrorHandlerOff;
/* The attached-media identity is supplied by disc_check.c in production.
 * Independently control the guest's selected header word and selector return
 * so neither can stand in for media identity in this bridge fixture. */
enum { DISC1_FILE1_BASE = 3087, DISC2_FILE1_BASE = 3082 };
static int mounted_disc;
static uint32_t selected_word, selector_return;
static int switch_media_during_read;
static uint32_t replace_word_during_read;
int PcPort_BattleArchiveDisc(void) { return mounted_disc; }
{ UNUSED_ARGS;w32(guest(0x8004fe14u),selected_word);return selector_return; }
static void archive_wrapper(void)
{ w32(guest(0x8004fe14u),selected_word); }
 if(switch_media_during_read)mounted_disc=switch_media_during_read;
 if(replace_word_during_read)w32(guest(0x8004fe14u),replace_word_during_read);
  {0x80034714,(void*)resident_queue,"func_80034714"},
  {0x8008ab4c,(void*)archive_wrapper,"func_8008AB4C"},
  {0x8008ab70,(void*)archive_wrapper,"func_8008AB70"},
  {0x8008ab94,(void*)archive_wrapper,"func_8008AB94"}
 mounted_disc=1;selected_word=selector_return=DISC1_FILE1_BASE;
 switch_media_during_read=0;replace_word_during_read=0;
 w32(guest(0x8004fe48),0);w32(guest(0x8004fe14),DISC1_FILE1_BASE);
static int select_module(uint32_t target,uint32_t directory,uint32_t subdirectory)
{
 test_parent.gpr[4]=directory;test_parent.gpr[5]=subdirectory;
 return runtime_bridge(&test_runtime,&test_parent,target)==1;
}
static int read_module(uint32_t index,uint32_t destination,uint32_t mode,uint32_t flags)
 test_parent.gpr[6]=mode;test_parent.gpr[7]=flags;
static int load_module(uint32_t directory,uint32_t index,uint32_t destination)
{
 return select_module(0x80028470u,directory,0) && read_module(index,destination,0,0x80);
}
static int one_variant(const Case *c,unsigned rebind,int nested,int disc)
 uint32_t base=disc==1 ? DISC1_FILE1_BASE : DISC2_FILE1_BASE;
 init_common(c);w32(guest(0x8004fe48),0);w32(guest(0x8004fe14),base);trace=&raw;rebind_flags=rebind&7;
 init_common(c);prepare_runtime();mounted_disc=disc;selected_word=selector_return=base;trace=&native;
static int one(const Case *c) { return one_variant(c,0,0,1); }
static int archive_gate_tests(void)
{
 const uint32_t direct=0x80028470u, wrapper=0x8008ab4cu;
 const struct Selection {
  int disc;
  uint32_t target,directory,subdirectory,word,result;
  int expected;
  const char *label;
 } selections[]={
  {1,direct,0x20,0,3087,3087,1,"disc 1 valid direct selector"},
  {2,direct,0x20,0,3082,3082,1,"disc 2 valid direct selector"},
  {1,wrapper,0,0,3087,0,1,"disc 1 valid wrapper"},
  {2,wrapper,0,0,3082,0,1,"disc 2 valid wrapper"},
  {0,direct,0x20,0,3087,3087,0,"absent or unauthenticated medium"},
  {-1,direct,0x20,0,3082,3082,0,"invalid media identity"},
  {3,direct,0x20,0,3082,3082,0,"unsupported game disc"},
  {1,direct,0x20,0,3082,3082,0,"disc 2 header with disc 1 media"},
  {2,direct,0x20,0,3087,3087,0,"disc 1 header with disc 2 media"},
  {1,direct,0x20,0,3087,3082,0,"wrong selector result on disc 1"},
  {2,direct,0x20,0,3082,3087,0,"wrong selector result on disc 2"},
  {2,direct,0x20,0,3082,UINT32_MAX,0,"failed selector on disc 2"},
  {1,direct,0x20,0,3079,3079,0,"unsupported file table base"},
  {2,direct,0x21,0,3082,3082,0,"wrong directory forged valid base"},
  {2,direct,0x1f,1,3082,3082,0,"different selector addresses same header entry"},
  {2,direct,0x20,2,3082,3082,0,"wrong subdirectory forged valid base"},
  {2,direct,0x20,2,3087,3087,0,"disc 2 subdirectory 2 aliases disc 1 base"},
  {1,0x8008ab70u,0,0,3087,0,0,"wrapper subdirectory 2 forged valid on disc 1"},
  {2,0x8008ab70u,0,0,3082,0,0,"wrapper subdirectory 2 forged valid on disc 2"},
  {2,0x8008ab94u,0,0,3082,0,0,"wrapper subdirectory 3 forged valid on disc 2"},
 };
 for(size_t i=0;i<sizeof(selections)/sizeof(*selections);i++){
  const struct Selection *v=&selections[i];
  init_common(&simple);prepare_runtime();
  /* Start valid cases unarmed. For every negative, first prove a valid
   * selection so the next selector has to discard stale authority. */
  if(!v->expected){
   mounted_disc=v->disc==1?1:2;
   selected_word=selector_return=mounted_disc==1?DISC1_FILE1_BASE:DISC2_FILE1_BASE;
   CHECK("prime selector",select_module(direct,0x20,0));
  }
  mounted_disc=v->disc;selected_word=v->word;selector_return=v->result;
  CHECK("fixture selector crossed bridge",select_module(v->target,v->directory,v->subdirectory));
  CHECK("fixture full file read crossed bridge",read_module(1,MODULE_ADDR,0,0x80));
  CHECK(v->label,test_runtime.file1.verified==v->expected);
  if(!v->expected)CHECK("unproven file uses fallback",runtime_bridge(&test_runtime,&test_parent,ENTRY)==0);
 }
 for(int disc=1;disc<=2;disc++){
  uint32_t base=disc==1 ? DISC1_FILE1_BASE : DISC2_FILE1_BASE;
  uint32_t other=disc==1 ? DISC2_FILE1_BASE : DISC1_FILE1_BASE;
  for(unsigned fault=0;fault<14;fault++){
   init_common(&simple);prepare_runtime();
   mounted_disc=disc;selected_word=selector_return=base;
   CHECK("valid selection before fault",select_module(direct,0x20,0));
   uint32_t size=MODULE_SIZE,mode=0,flags=0x80,index=1,destination=MODULE_ADDR;
   switch(fault){
    case 0: mounted_disc=3-disc;break;
    case 1: w32(guest(0x8004fe14u),other);break;
    case 2: mounted_disc=3-disc;w32(guest(0x8004fe14u),other);break;
    case 3: switch_media_during_read=3-disc;break;
    case 4: replace_word_during_read=other;break;
    case 5: archive_failure=1;break;
    case 6: archive_corrupt=1;break;
    case 7: size=MODULE_SIZE-4;break;
    case 8: size=MODULE_SIZE+4;break;
    case 9: index=2;break;
    case 10: mode=1;break;
    case 11: flags=0x40;break;
    case 12: destination=MODULE_ADDR+4;break;
    case 13: w32(guest(0x8004fe48u),1);break;
   }
   archive_size=size;
   CHECK("faulted file read crossed bridge",read_module(index,destination,mode,flags));
   if(test_runtime.file1.verified)fprintf(stderr,"disc=%d fault=%u\n",disc,fault);
   CHECK("changed media/header, unsuccessful full read or caller tuple cannot authorize",!test_runtime.file1.verified);
   CHECK("unverified entry fallback",runtime_bridge(&test_runtime,&test_parent,ENTRY)==0);
  }
  init_common(&simple);prepare_runtime();mounted_disc=disc;selected_word=selector_return=base;
  CHECK("valid file before observed identical write",load_module(0x20,1,MODULE_ADDR)&&test_runtime.file1.verified);
  CHECK("observed same immutable byte cannot be a fresh load",runtime_write(&test_runtime,MODULE_ADDR,1,module_bytes[0])==0 &&
        !test_runtime.file1.verified && runtime_bridge(&test_runtime,&test_parent,ENTRY)==0);
  CHECK("legitimate reload before unobserved change",load_module(0x20,1,MODULE_ADDR)&&test_runtime.file1.verified);
  guest(MODULE_ADDR)[0]^=1;
  CHECK("changed immutable region rejects entry on either disc",runtime_bridge(&test_runtime,&test_parent,ENTRY)==0&&!test_runtime.file1.verified);
 }
 return 1;
}
 CHECK("two-disc archive load gates",archive_gate_tests());
 if(!one_variant(&simple,0,0,2))return 0;
 for(unsigned bits=1;bits<8;bits++)if(!one_variant(&simple,bits,0,1))return 0;
 if(!one_variant(&simple,0,1,1))return 0;
 if(!one_variant(&closed,0x80,0,1))return 0;
 if(!one_variant(&wrapped,0,0,1))return 0;
