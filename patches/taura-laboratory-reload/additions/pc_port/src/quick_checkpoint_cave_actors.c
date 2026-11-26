#include "field/camera.h"
    if (D_800AFB54 == 2) /* The laboratory has a single walkable layer. */
        return layer == 0 && triangles[0] < D_800AFB44[0] &&
               !triangles[1] && !triangles[2] && !triangles[3];
static int follower_pair_bound(int leader, int partner, int character)
           follower->defaultAnimationId == 0;
}

static int follower_pair_ready(int leader, int partner, int character)
{
    return D_800AFB54 == 3 && D_800AFB44[0] > 0 && D_800AFB44[1] > 0 &&
           follower_pair_bound(leader, partner, character) && actor_data(partner)->curAnimationId == 0;
    u8 animation, slope_contact; /* F616 only; F614 and W615 leave both zero. */
enum EllyRoom { ELLY_FOREST, ELLY_WORKSHOP, ELLY_LAB };
static const char elly_tags[][5] = { "F614", "W615", "F616" };
enum { ACTOR_SLOPE_CONTACT = 0x04000000 };
typedef struct {
    s16 rotation[3], sprite_direction, sprite_facing;
    u16 culled;
} LabResidentFacing;

enum { LAB_CITAN = 14, LAB_TAURA = 16, LAB_RESIDENTS = 2, LAB_CULLED = 0x200 };
/* Earlier F616 records left this byte zero. Their ground contact is unknown. */
enum { LAB_CONTACT_UNSAVED, LAB_CONTACT_CLEAR, LAB_CONTACT_SET };
enum { LAB_FEI_CONTACT_OFFSET = sizeof(EllyParty) + LAB_RESIDENTS * sizeof(LabResidentFacing) };
static const u8 lab_residents[LAB_RESIDENTS] = { LAB_CITAN, LAB_TAURA };
_Static_assert(sizeof(LabResidentFacing) == 12 && LAB_FEI_CONTACT_OFFSET == 1764 &&
               LAB_FEI_CONTACT_OFFSET < PC_PORT_QUICK_PARTY_BYTES, "F616 party layout");

static int lab_raw_heading_valid(u16 angle)
{
    /* Script turns retain the high direction flag and can sign-extend a
     * negative 12-bit angle; do not strip those bits from the loaded pose. */
    u16 high = angle & 0xf000u;
    return high == 0 || high == 0x8000 || high == 0xf000;
}

static int lab_facings_valid(const LabResidentFacing facings[LAB_RESIDENTS])
{
    for (unsigned i = 0; i < LAB_RESIDENTS; ++i) {
        const LabResidentFacing *f = &facings[i];
        if (!lab_raw_heading_valid((u16)f->rotation[0]) || !lab_raw_heading_valid((u16)f->rotation[1]) ||
            (u16)f->rotation[2] > 0xfff || f->sprite_direction < -0x1000 || f->sprite_direction > 0xfff ||
            f->sprite_facing < -0x1000 || f->sprite_facing > 0x1ffe || (f->culled & ~LAB_CULLED))
            return 0;
    }
    return 1;
}

static void transfer_lab_facings(LabResidentFacing facings[LAB_RESIDENTS], int restore)
{
    for (unsigned i = 0; i < LAB_RESIDENTS; ++i) {
        const unsigned slot = lab_residents[i];
        ActorData *npc = actor_data(slot);
        u8 *sprite = (u8 *)(uintptr_t)g_FieldActors[slot].pSpriteData;
        LabResidentFacing *f = &facings[i];
        transfer(f->rotation, &npc->rotation.vx, sizeof f->rotation, restore);
        transfer(&f->sprite_direction, sprite + 0x32, sizeof f->sprite_direction, restore);
        if (restore) {
            npc->flags = (npc->flags & ~LAB_CULLED) | f->culled;
            func_800223B0(sprite, f->sprite_facing);
        } else {
            memcpy(&f->sprite_facing, sprite + 0x80, sizeof f->sprite_facing);
            f->culled = npc->flags & LAB_CULLED;
        }
    }
}

/* The field loader separately authenticates the Disc 2 script and actor 19's
 * completed meeting. This is Elly's ordinary postmeeting party binding. */
static int taura_lab_party_ready(void)
{
    if ((g_GameSceneMapNum & 0x3fff) != 616 || g_FieldScriptMemoryU16[0] != 249 ||
        g_FieldScriptMemoryU16[1] != 0 || g_FieldScriptMemoryU16[2] != 615 ||
        (g_FieldScriptMemoryU16[0x1c0 / 2] & 0x1c00) != 0x1400 || D_800ADBFC != 34 ||
        g_FieldNumActors < 34 || D_800AFB54 != 2 || D_800AFB44[0] <= 0 ||
        !follower_pair_bound(WORKSHOP_FEI, WORKSHOP_ELLY, 1) || !g_FieldActors[WORKSHOP_FEI].pSpriteData)
        return 0;
    for (unsigned i = 0; i < LAB_RESIDENTS; ++i) {
        const unsigned slot = lab_residents[i];
        const FieldActor *field = &g_FieldActors[slot];
        const ActorData *npc = actor_data(slot);
        if (!npc || !field->pSpriteData || (field->status & (0xf80u | ACTOR_STATUS_INVISIBLE)) != 0x200 ||
            npc->characterId != 255 || npc->scriptFlags.flags & 1)
            return 0;
    }
    const ActorData *elly = actor_data(WORKSHOP_ELLY);
    return elly->curAnimationId >= 0 && elly->curAnimationId <= 3;
}

static int capture_elly_party(uint8_t *out, enum EllyRoom room, PcPortForestCameraTiming timing)
    memcpy(saved.tag, elly_tags[room], sizeof saved.tag);
    if (room == ELLY_LAB) {
        saved.animation = (u8)elly->curAnimationId;
        saved.slope_contact = !!(elly->scriptFlags.flags & ACTOR_SLOPE_CONTACT);
    }
    LabResidentFacing facings[LAB_RESIDENTS];
    if (room == ELLY_LAB) {
        transfer_lab_facings(facings, 0);
        if (!lab_facings_valid(facings))
            return 0;
    }
    if (room == ELLY_LAB) {
        memcpy(out + sizeof saved, facings, sizeof facings);
        const ActorData *fei = actor_data(WORKSHOP_FEI);
        out[LAB_FEI_CONTACT_OFFSET] = (fei->scriptFlags.flags & ACTOR_SLOPE_CONTACT)
                                    ? LAB_CONTACT_SET : LAB_CONTACT_CLEAR;
    }
static int restore_elly_party(const uint8_t *in, enum EllyRoom room, PcPortForestCameraTiming *timing)
    if (memcmp(saved.tag, elly_tags[room], sizeof saved.tag) ||
        (room == ELLY_LAB ? saved.animation > 3 || saved.slope_contact > 1 :
                            saved.animation || saved.slope_contact) ||
    size_t used = sizeof saved;
    LabResidentFacing facings[LAB_RESIDENTS];
    u8 fei_contact = LAB_CONTACT_UNSAVED;
    if (room == ELLY_LAB) {
        memcpy(facings, in + used, sizeof facings);
        fei_contact = in[LAB_FEI_CONTACT_OFFSET];
        if (!lab_facings_valid(facings) || fei_contact > LAB_CONTACT_SET)
            return 0;
        used = LAB_FEI_CONTACT_OFFSET + 1;
    }
    for (size_t i = used; i < PC_PORT_QUICK_PARTY_BYTES; ++i)
    if (room == ELLY_LAB) {
        if (fei_contact != LAB_CONTACT_UNSAVED) {
            ActorData *fei = actor_data(WORKSHOP_FEI);
            fei->scriptFlags.flags = (fei->scriptFlags.flags & ~ACTOR_SLOPE_CONTACT) |
                                     (fei_contact == LAB_CONTACT_SET ? ACTOR_SLOPE_CONTACT : 0);
        }
        transfer_lab_facings(facings, 1);
        /* Rebinding can clear the moving flag. Restore the saved movement and
         * sprite-facing values after the live animation owns its resources. */
        if (elly->curAnimationId != saved.animation) {
            elly->curAnimationId = saved.animation;
            func_800821F4((void *)(uintptr_t)g_FieldActors[WORKSHOP_ELLY].pSpriteData,
                          saved.animation, &g_FieldActors[WORKSHOP_ELLY]);
        }
        elly->scriptFlags.flags = (elly->scriptFlags.flags & ~ACTOR_SLOPE_CONTACT) |
                                  (saved.slope_contact ? ACTOR_SLOPE_CONTACT : 0);
    }
           capture_elly_party(out, ELLY_FOREST, timing);
    return in && timing && workshop_party_ready() && restore_elly_party(in, ELLY_FOREST, timing);
           capture_elly_party(out, ELLY_WORKSHOP, (PcPortForestCameraTiming){0});
    return in && taura_workshop_party_ready() && restore_elly_party(in, ELLY_WORKSHOP, NULL);
}

int PcPort_TauraPartyCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES])
{
    return out && taura_lab_party_ready() &&
           capture_elly_party(out, ELLY_LAB, (PcPortForestCameraTiming){0});
}

int PcPort_TauraPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES])
{
    return in && taura_lab_party_ready() && restore_elly_party(in, ELLY_LAB, NULL);
int PcPort_TauraPartyRegroup(void)
{
    if (!taura_lab_party_ready() || !group_follower(WORKSHOP_FEI, WORKSHOP_ELLY))
        return 0;
    /* Legacy FCV1 has no follower offset or trail. At the meeting handback
     * Elly is standing on Fei facing him, independently of his own yaw.
     * For later saved positions retain the field's normal regroup. */
    const ActorData *fei = actor_data(WORKSHOP_FEI);
    if (fei->position.vx == -393 * 65536 && fei->position.vy == 30 * 65536 &&
        fei->position.vz == -177 * 65536 && (u16)fei->rotation.vy == 0xfffe) {
        ActorData *elly = actor_data(WORKSHOP_ELLY);
        elly->rotation.vx = elly->rotation.vy = (s16)0x8400;
        elly->rotation.vz = 0x400;
        elly->scriptFlags.flags |= ACTOR_SLOPE_CONTACT;
        u8 *sprite = (u8 *)(uintptr_t)g_FieldActors[WORKSHOP_ELLY].pSpriteData;
        s16 direction = (u16)fei->rotation.vy & 0xfff;
        memcpy(sprite + 0x32, &direction, sizeof direction);
        func_800223B0(sprite, (s16)(g_CamInterpolation.curAngleY + elly->rotation.vz));
    }
    return 1;
}

