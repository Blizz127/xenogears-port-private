#define main bridge_original_main
#include "hd2d_bridge_test.c"
#undef main
#include <stdlib.h>
static int loaded,saves;
static char saved_key[64],saved_value[16];
extern void PcPort_Hd2dApplySettings(void);
const char *xg_plat_config_get(const char *key)
{
    if (!loaded) return NULL;
    if (!strcmp(key,"enhancements.hd_2d__octopath_style_")) return "1";
    if (!strcmp(key,"enhancements.bloom___glow")) return "0";
    if (!strcmp(key,"enhancements.bloom_intensity")) return "71";
    if (!strcmp(key,"enhancements.fog_intensity")) return "999";
    if (!strcmp(key,"enhancements.god_ray_intensity")) return "bad";
    if (!strcmp(key,"enhancements.warm_lighting_intensity")) return "9999999999999999999999999999999999999999";
    if (!strcmp(key,"enhancements.particle_intensity")) return "-9999999999999999999999999999999999999999";
    return NULL;
}
int xg_plat_config_persist(const char *key,const char *value)
{
    ++saves; snprintf(saved_key,sizeof saved_key,"%s",key);
    snprintf(saved_value,sizeof saved_value,"%s",value); return 1;
}
int main(void)
{
    /* Simulate an early constructor getter before the file is loaded. */
    const char *override=getenv("XENO_HD2D_OCTOPATH");
    assert(PcPort_Hd2dEnabled()==(override && !strcmp(override,"1")));
    loaded=!getenv("HD2D_SETTINGS_ABSENT");
    /* Menu loads numeric rows before runtime apply, without writes. */
    if (loaded) controls[XG_HD2D_BLOOM].set(71);
    assert(!saves); PcPort_Hd2dApplySettings();
    assert(PcPort_Hd2dEnabled()==(override ? !strcmp(override,"1") : loaded));
    const XgHd2dConfig *c=PcPort_Hd2dConfiguration();
    assert(c->effect[XG_HD2D_BLOOM].enabled==!loaded);
    assert(fabsf(c->effect[XG_HD2D_BLOOM].intensity-(loaded ? .71f : .25f))<.00001f);
    assert(c->effect[XG_HD2D_FOG].intensity==(loaded ? 1 : .25f));
    assert(c->effect[XG_HD2D_RAYS].intensity==.25f);
    assert(c->effect[XG_HD2D_LIGHT].intensity==.25f);
    assert(c->effect[XG_HD2D_PARTICLES].intensity==.25f);
    assert(!saves && !gl_calls && !producer_calls);
    PcPort_Hd2dApplySettings(); assert(!saves);
    toggle_controls[0]();
    assert(saves==1 && !strcmp(saved_key,"enhancements.hd_2d__octopath_style_"));
    const char *keys[]={"depth_of_field___tilt_shift","bloom___glow","warm_lighting","sprite_shadows","vignette","fog","particles","colour_grading","god_rays"};
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e) {
        int before=c->effect[e].enabled;
        toggle_controls[e+1]();
        assert(c->effect[e].enabled==!before && saves==e+2);
        char key[64]; snprintf(key,sizeof key,"enhancements.%s",keys[e]);
        assert(!strcmp(saved_key,key));
        assert(!strcmp(saved_value,before ? "0" : "1"));
    }
    const char *intensity_names[]={"depth_of_field_intensity","bloom_intensity","warm_lighting_intensity","sprite_shadow_intensity","vignette_intensity","fog_intensity","particle_intensity","colour_grading_intensity","god_ray_intensity"};
    for (int e=0;e<XG_HD2D_EFFECT_COUNT;++e) {
        int before=saves; char key[64];
        assert(PcPort_Hd2dSetIntensity(e,.42f));
        assert(saves==before+1);
        snprintf(key,sizeof key,"enhancements.%s",intensity_names[e]);
        assert(!strcmp(saved_key,key) && !strcmp(saved_value,"42"));
        assert(PcPort_Hd2dSetIntensity(e,.42f));
        assert(!PcPort_Hd2dSetIntensity(e,NAN));
        assert(!PcPort_Hd2dSetIntensity(e,INFINITY));
        assert(!PcPort_Hd2dSetIntensity(e,-.1f));
        assert(!PcPort_Hd2dSetIntensity(e,1.1f));
        assert(saves==before+1);
    }
    int before=saves;
    assert(!PcPort_Hd2dSetIntensity(-1,.5f));
    assert(!PcPort_Hd2dSetIntensity(XG_HD2D_EFFECT_COUNT,.5f));
    assert(saves==before);
    puts("HD2D settings: early init, startup restore, no startup writes, env precedence, toggle persistence PASS");
    return 0;
}
