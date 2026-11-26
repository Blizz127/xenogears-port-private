/* Test-only US Disc 1 CBA2C. Step its camera once from an observed
 * VBlank and keep both full node banks for the native carried-pose test. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"

static uint8_t image[0x200000];
static uint32_t menu, nodes;
static unsigned yawWrites;

static int read_guest(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    (void)opaque;
    if (address < 0x80000000u || address - 0x80000000u > sizeof(image) - width) return -1;
    *value = 0;
    memcpy(value, image + (address - 0x80000000u), width);
    return 0;
}

static uint32_t read_value(uint32_t address, unsigned width)
{
    uint32_t value;
    assert(!read_guest(NULL,address,width,&value));
    return value;
}

static int write_guest(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque;
    if (width == 2 && address == nodes + 0x56) {
        ++yawWrites;
    } else if (!(width == 4 && ((address >= 0x801D9074u && address <= 0x801D907Cu &&
                                  !(address & 3)) || address == menu+0x224 || address == menu+0x228)) &&
               !(width == 1 && address == menu+0x329)) {
        fprintf(stderr,"unexpected write at %08x width %u\n",address,width);
        return -1;
    }
    memcpy(image + (address - 0x80000000u), &value, width);
    return 0;
}

static int trace(void *opaque, PcPortMipsCpu *cpu, uint32_t pc)
{
    (void)opaque;
    (void)cpu;
    return pc >= 0x801CBA2Cu && pc < 0x801CBDA0u ? 0 : -1;
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    FILE *f = fopen(argv[1], "rb");
    assert(f && fread(image,1,sizeof(image),f) == sizeof(image) && fgetc(f) == EOF && !fclose(f));
    menu = read_value(0x800625A0u,4);
    nodes = read_value(read_value(0x801E8674u,4)+4,4);
    assert(read_value(menu+0x329,1) & 4);
    static uint8_t prior[52*0x7c];
    assert(nodes >= 0x80000000u && nodes-0x80000000u <= sizeof(image)-sizeof(prior));
    const unsigned char *bank = image + (nodes - 0x80000000u);
    memcpy(prior,bank,sizeof(prior));
    int yaw = (int16_t)read_value(nodes+0x56,2);
    PcPortMipsBus bus = {.read=read_guest,.write=write_guest,.bridge=trace};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[29]=0x801ff000u;
    cpu.gpr[31]=0xfffffffcu;
    int rc=PcPortMipsRun(&cpu,0x801CBA2Cu,0xfffffffcu,1024);
    if(rc!=PC_PORT_MIPS_HALTED) fprintf(stderr,"CBA2C at %08x: %s\n",cpu.pc,cpu.error);
    assert(rc==PC_PORT_MIPS_HALTED && yawWrites==1);
    assert(!memcmp(prior,bank,0x56) && !memcmp(prior+0x58,bank+0x58,sizeof(prior)-0x58));
    int nextYaw=(int16_t)read_value(nodes+0x56,2);
    assert(yaw!=nextYaw);
    f=fopen(argv[2],"wbx");
    assert(f && fwrite(image,1,sizeof(image),f)==sizeof(image) && !fclose(f));
    printf("%d %d\n",yaw,nextYaw);
    return 0;
}
