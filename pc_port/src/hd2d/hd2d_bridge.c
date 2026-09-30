/* Landed hook integration. New master OFF never touches game or GL state. */
#include "hd2d_integration.h"
#include "hd2d_sprite.h"
#include "hd2d_buffers_adapter.h"
#include "port_dev_menu.h"
#include "quick_checkpoint.h"
#include "hd2d_sprite_profile.h"
#include <stdio.h>
#include <errno.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern int g_GameSceneMapNum;
/* Weak numeric-menu ABI keeps isolated/no-registry builds linkable. */
extern void PcPort_DevMenuRegisterEnhancementValue(const char*,int(*)(void),
    void(*)(int),int,int,int,const char*) __attribute__((weak));
#pragma weak PcPort_Hd2dSceneInputs
#pragma weak PcPort_Hd2dSceneItemsReset
extern void PcPort_Hd2dSceneItemsReset(void);
#pragma weak PcPort_Hd2dSceneContactsReset
extern void PcPort_Hd2dSceneContactsReset(void);
extern void PcPort_Hd2dSpritePresentDone(void) __attribute__((weak));
extern void PcPort_Hd2dSpriteReset(void) __attribute__((weak));
/* Weak config ABI preserves isolated renderer tests and optional port builds. */
extern const char *xg_plat_config_get(const char *) __attribute__((weak));
extern int xg_plat_config_persist(const char *,const char *) __attribute__((weak));
static const char *effect_keys[XG_HD2D_EFFECT_COUNT]={
    "depth_of_field___tilt_shift", "bloom___glow", "warm_lighting",
    "sprite_shadows", "vignette", "fog", "particles", "colour_grading", "god_rays"
};
static const char *intensity_keys[XG_HD2D_EFFECT_COUNT]={
    "depth_of_field_intensity", "bloom_intensity", "warm_lighting_intensity",
    "sprite_shadow_intensity", "vignette_intensity", "fog_intensity",
    "particle_intensity", "colour_grading_intensity", "god_ray_intensity"
};
static const char *master_key="hd_2d__octopath_style_";
static int applying_settings, settings_applied;
static void settings_key(char key[64],const char *name)
{ snprintf(key,64,"enhancements.%s",name); }
static int stored(const char *name,int fallback,int max)
{
    char key[64],*end; long value; const char *text;
    if (!xg_plat_config_get) return fallback;
    settings_key(key,name); text=xg_plat_config_get(key);
    if (!text || !*text) return fallback;
    if (!strcmp(text,"on") || !strcmp(text,"true") || !strcmp(text,"yes")) return 1;
    if (!strcmp(text,"off") || !strcmp(text,"false") || !strcmp(text,"no")) return 0;
    errno=0; value=strtol(text,&end,0);
    if (errno==ERANGE || end==text || *end) return fallback;
    return value<0 ? 0 : value>max ? max : (int)value;
}
static void persist(const char *name,int value)
{
    char key[64],text[16];
    if (applying_settings || !xg_plat_config_persist) return;
    settings_key(key,name); snprintf(text,sizeof text,"%d",value);
    if (!xg_plat_config_persist(key,text)) fprintf(stderr,"[hd2d] could not save %s\n",key);
}
static XgHd2dConfig config;
static int initialized;

static void initialize(void)
{
    if (initialized) return;
    initialized=1;
    XgHd2d_DefaultConfig(&config);
    /* Verified private checkpoints: Fei's house and Lahan village. */
    config.maps[0]=14; config.maps[1]=1; config.map_count=2;
    for (int e=0;e<XG_HD2D_EFFECT_COUNT;++e) config.effect[e].enabled=1;
    /* Private diagnostic override; production opening maps need approval. */
    const char *maps=getenv("XENO_HD2D_OPENING_MAPS");
    if (maps && *maps) {
        int parsed[XG_HD2D_MAX_MAPS]; unsigned count=0; const char *p=maps;
        while (*p && count<XG_HD2D_MAX_MAPS) {
            char *end; long v=strtol(p,&end,10);
            if (end==p || v<0 || v>=1024 || (*end && *end!=',')) { count=0; break; }
            parsed[count++]=(int)v; p=*end ? end+1 : end;
            if (*end && !*p) { count=0; break; }
        }
        if (!*p && count) PcPort_Hd2dSetOpeningMaps(parsed,count);
    }
    const char *on=getenv("XENO_HD2D_OCTOPATH");
    config.enabled=on && !strcmp(on,"1");
}
int PcPort_Hd2dEnabled(void) { initialize(); return config.enabled; }
void PcPort_Hd2dSetEnabled(int enabled)
{
    initialize(); enabled=!!enabled;
    int changed=config.enabled!=enabled;
    if (config.enabled!=enabled && PcPort_Hd2dSpriteReset)
        PcPort_Hd2dSpriteReset(); /* CPU-only packet lifetime invalidation. */
    if (config.enabled!=enabled) PcPort_Hd2dBuffersReset();
    if (config.enabled!=enabled && PcPort_Hd2dSceneItemsReset) PcPort_Hd2dSceneItemsReset();
    if (config.enabled!=enabled && PcPort_Hd2dSceneContactsReset) PcPort_Hd2dSceneContactsReset();
    if (changed && !applying_settings) fprintf(stderr,"[hd2d] %s\n",enabled ? "enabled" : "disabled");
    config.enabled=enabled;
    if (changed) persist(master_key,enabled);
}
const XgHd2dConfig *PcPort_Hd2dConfiguration(void) { initialize(); return &config; }
int PcPort_Hd2dSetIntensity(int e,float value)
{
    initialize();
    if (e<0 || e>=XG_HD2D_EFFECT_COUNT || !isfinite(value) || value<0 || value>1) return 0;
    if (config.effect[e].intensity==value) return 1;
    config.effect[e].intensity=value;
    persist(intensity_keys[e],(int)(value*100.0f+0.5f));
    return 1;
}
int PcPort_Hd2dSetOpeningMaps(const int *maps,unsigned count)
{
    initialize();
    if (!maps || !count || count>XG_HD2D_MAX_MAPS) return 0;
    for (unsigned i=0;i<count;++i) {
        if (maps[i]<0 || maps[i]>=1024) return 0;
        for (unsigned j=0;j<i;++j) if (maps[i]==maps[j]) return 0;
    }
    memcpy(config.maps,maps,count*sizeof(*maps)); config.map_count=count; return 1;
}
int PcPort_Hd2dCurrentMap(void) { return g_GameSceneMapNum & 0x3fff; }
int PcPort_Hd2dOpeningActive(void)
{
    if (!PcPort_Hd2dEnabled()) return 0;
    if (!PcPort_QuickCheckpointFieldIsActive()) return 0;
    int map=PcPort_Hd2dCurrentMap();
    for (unsigned i=0;i<config.map_count;++i) if (config.maps[i]==map) return 1;
    return 0;
}
static void toggle_master(void) { PcPort_Hd2dSetEnabled(!PcPort_Hd2dEnabled()); }
#define CONTROL(NAME,INDEX) \
    static int get_##NAME(void) { initialize(); return config.effect[INDEX].enabled; } \
    static void toggle_##NAME(void) { initialize(); config.effect[INDEX].enabled^=1; persist(effect_keys[INDEX],config.effect[INDEX].enabled); fprintf(stderr,"[hd2d] %s %s\n",effect_keys[INDEX],config.effect[INDEX].enabled ? "enabled" : "disabled"); } \
    static int get_##NAME##_percent(void) { initialize(); return (int)(config.effect[INDEX].intensity*100.0f+0.5f); } \
    static void set_##NAME##_percent(int value) { int previous=applying_settings; if (!settings_applied) applying_settings=1; if(value>=0 && value<=100) PcPort_Hd2dSetIntensity(INDEX,value/100.0f); applying_settings=previous; }
CONTROL(dof,XG_HD2D_DOF)
CONTROL(bloom,XG_HD2D_BLOOM)
CONTROL(light,XG_HD2D_LIGHT)
CONTROL(shadow,XG_HD2D_SHADOW)
CONTROL(vignette,XG_HD2D_VIGNETTE)
CONTROL(fog,XG_HD2D_FOG)
CONTROL(particles,XG_HD2D_PARTICLES)
CONTROL(grade,XG_HD2D_GRADE)
CONTROL(rays,XG_HD2D_RAYS)
__attribute__((constructor)) static void register_menu(void)
{
    PcPort_DevMenuRegisterEnhancement("HD-2D (Octopath style)",PcPort_Hd2dEnabled,toggle_master);
    PcPort_DevMenuRegisterEnhancement("Depth of field / tilt shift",get_dof,toggle_dof);
    PcPort_DevMenuRegisterEnhancement("Bloom / glow",get_bloom,toggle_bloom);
    PcPort_DevMenuRegisterEnhancement("Warm lighting",get_light,toggle_light);
    PcPort_DevMenuRegisterEnhancement("Sprite shadows",get_shadow,toggle_shadow);
    PcPort_DevMenuRegisterEnhancement("Vignette",get_vignette,toggle_vignette);
    PcPort_DevMenuRegisterEnhancement("Fog",get_fog,toggle_fog);
    PcPort_DevMenuRegisterEnhancement("Particles",get_particles,toggle_particles);
    PcPort_DevMenuRegisterEnhancement("Colour grading",get_grade,toggle_grade);
    PcPort_DevMenuRegisterEnhancement("God rays",get_rays,toggle_rays);
    if(PcPort_DevMenuRegisterEnhancementValue) {
#define VALUE(LABEL,NAME) PcPort_DevMenuRegisterEnhancementValue(LABEL,get_##NAME##_percent,set_##NAME##_percent,0,100,5,"%")
        VALUE("Depth of field intensity",dof);
        VALUE("Bloom intensity",bloom);
        VALUE("Warm lighting intensity",light);
        VALUE("Sprite shadow intensity",shadow);
        VALUE("Vignette intensity",vignette);
        VALUE("Fog intensity",fog);
        VALUE("Particle intensity",particles);
        VALUE("Colour grading intensity",grade);
        VALUE("God ray intensity",rays);
#undef VALUE
    }
}
/* Called after CLI/file load, regardless of constructor/menu ordering. */
void PcPort_Hd2dApplySettings(void)
{
    if (settings_applied) return;
    initialize(); applying_settings=1;
    for (int e=0;e<XG_HD2D_EFFECT_COUNT;++e) {
        config.effect[e].enabled=stored(effect_keys[e],config.effect[e].enabled,1);
        config.effect[e].intensity=stored(intensity_keys[e],
            (int)(config.effect[e].intensity*100.0f+0.5f),100)/100.0f;
    }
    const char *on=getenv("XENO_HD2D_OCTOPATH");
    PcPort_Hd2dSetEnabled(on ? !strcmp(on,"1") : stored(master_key,0,1));
    applying_settings=0; settings_applied=1;
    size_t frames=0;
    for (size_t i=0;i<hd2d_sprite_profile.count;++i) {
        const PcPortHd2dPartMapping *a=&hd2d_sprite_profile.parts[i]; size_t j;
        for (j=0;j<i;++j) {
            const PcPortHd2dPartMapping *b=&hd2d_sprite_profile.parts[j];
            if (a->identity==b->identity && a->actor==b->actor && a->frame==b->frame) break;
        }
        if (j==i) ++frames;
    }
    const char *directory=getenv("XENO_HD2D_ATLAS_DIR");
    const char *path=hd2d_sprite_profile.rgba_path;
    const char *name=strrchr(path,'/'); name=name ? name+1 : path;
    if (directory && *directory)
        fprintf(stderr,"[hd2d] startup enabled=%d atlas=%s/%s mapped_frames=%zu\n",config.enabled,directory,name,frames);
    else fprintf(stderr,"[hd2d] startup enabled=%d atlas=%s mapped_frames=%zu\n",config.enabled,path,frames);
}
void PcPort_Hd2dPostProcess(int path,int width,int height)
{
    if (!PcPort_Hd2dEnabled()) return;
    /* Both landed hooks run after GPU submission has consumed this OT. */
    if (PcPort_Hd2dSpritePresentDone) PcPort_Hd2dSpritePresentDone();
    XgHd2dFrame frame={0};
    frame.field_active=PcPort_Hd2dOpeningActive();
    if (!frame.field_active || width<=0 || height<=0) {
        PcPort_Hd2dBuffersConsume(path,width,height,&frame);
        return;
    }
    frame.field_id=PcPort_Hd2dCurrentMap();
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
    if (viewport[0]<0 || viewport[1]<0 || viewport[2]<=0 || viewport[3]<=0 ||
        viewport[0]>width-viewport[2] || viewport[1]>height-viewport[3]) {
        PcPort_Hd2dBuffersConsume(path,width,height,&frame);
        return;
    }
    frame.x=viewport[0]; frame.y=viewport[1]; frame.width=viewport[2]; frame.height=viewport[3];
    /* Only HOOK4 certifies attachments. Items may use their explicit units. */
    PcPort_Hd2dBuffersConsume(path,width,height,&frame);
    if (path==0 && PcPort_Hd2dSceneInputs) {
        XgHd2dFrame produced=frame;
        if (PcPort_Hd2dSceneInputs(path,width,height,&produced) &&
            produced.field_active && produced.field_id==frame.field_id &&
            produced.x==frame.x && produced.y==frame.y &&
            produced.width==frame.width && produced.height==frame.height) {
            frame.light_count=produced.light_count;
            frame.shadow_count=produced.shadow_count;
            frame.particle_count=produced.particle_count;
            frame.particle_mode=produced.particle_mode;
            frame.shadow_mode=produced.shadow_mode;
            memcpy(frame.lights,produced.lights,sizeof frame.lights);
            memcpy(frame.shadows,produced.shadows,sizeof frame.shadows);
            memcpy(frame.shadow_order,produced.shadow_order,sizeof frame.shadow_order);
            memcpy(frame.particles,produced.particles,sizeof frame.particles);
        }
    }
    XgHd2d_Present(&config,&frame);
}
int PcPort_Hd2dTextureOverride(int tpage,int clut,int u0,int v0,int u1,int v1,
    unsigned *texture,float uv[4])
{
    (void)tpage; (void)clut; (void)u0; (void)v0; (void)u1; (void)v1;
    (void)texture; (void)uv;
    /* No retail-derived area pack or guessed UV association is supplied. */
    return 0;
}
