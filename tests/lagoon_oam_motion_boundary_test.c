/* Exercise the actual SDK history/evaluator, including parked-slot pixels.
 * Reuse the ROM-free SDK harness stubs; no copied history implementation. */
#define main sdk_sprite_suite_main
#define WsShadowTile legacy_harness_shadow_tile
#include "../snesrecomp/tests/ppu/ppu_sprite_limit_test.c"
#undef main
#undef WsShadowTile
uint16_t WsShadowTile(int layer, int screen_x, uint32_t wrapped_y,
                     uint16_t scroll, uint16_t address, uint16_t tile) {
    (void)layer; (void)screen_x; (void)wrapped_y;
    (void)scroll; (void)address; return tile;
}
#include "../snesrecomp/runner/src/snes/ppu.c"
#include <assert.h>

enum { Extra = 213, Width = 256 + 2 * Extra };
static uint32_t pixels[Width];
static const uint8_t hints[16];

static void reset_case(Ppu *p) {
    ppu_reset(p);
    PpuBeginDrawing(p, (uint8_t *)pixels, sizeof pixels,
                    kPpuRenderFlags_NewRenderer);
    PpuSetExtraSpace(p, Extra);
    PpuWsSetOamLeftHints(p, hints);
    PpuWsSetOamRightHints(p, hints);
}
static void field(Ppu *p, int x) {
    memset(pixels, 0, sizeof pixels);
    setup_one_sprite_line(p, x & 511);
    ppu_runLine(p, 0);
    ppu_runLine(p, 1);
}
static void prime(Ppu *p, int x) {
    reset_case(p);
    field(p, x + 5);
    field(p, x);
    assert(p->wsOamMotionGrace[0] == 4);
}
static void history_step(Ppu *p, int x) {
    setup_one_sprite_line(p, x & 511);
    PpuUpdateWidescreenOamHistory(p, 0);
    PpuUpdateWidescreenOamHistory(p, 1);
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--sdk-suite"))
        return sdk_sprite_suite_main();
    Ppu *p = ppu_init(); assert(p);
    int old = argc == 2 && !strcmp(argv[1], "--expect-old");
    assert(argc == 1 || old);
    prime(p, -90);
    unsigned parked_fields = 0;
    for (unsigned i = 0; i < 4; ++i) {
        field(p, 320);
        assert(PpuDecodeOamX(p, 0) == -192);
        parked_fields += pixels[Extra - 192] != 0;
        assert(p->wsOamMotionGrace[0] == (old ? 3 - i : 0));
    }
    assert(parked_fields == (old ? 3u : 0u));
    printf("Actual parked X320: %u stale visible fields after grace4\n", parked_fields);
    if (old) { ppu_free(p); return 0; }

    unsigned boundaries = 0;
    for (int dx = -40; dx <= 40; ++dx) {
        prime(p, -90);
        history_step(p, -90 + dx);
        unsigned expected = dx == 0 ? 3 : (dx > -32 && dx < 32 ? 4 : 0);
        assert(p->wsOamMotionGrace[0] == expected);
        /* Repeated scanlines are not new fields. */
        PpuUpdateWidescreenOamHistory(p, 1);
        assert(p->wsOamMotionGrace[0] == expected);
        ++boundaries;
    }
    prime(p, -90);
    for (unsigned i = 0; i < 4; ++i) {
        field(p, -90);
        assert(p->wsOamMotionGrace[0] == 3 - i);
        assert((pixels[Extra - 90] != 0) == (i < 3));
    }
    /* Same slot, changed tile/attributes/Y/size means changed ownership. */
    for (unsigned kind = 0; kind < 4; ++kind) {
        prime(p, -90);
        setup_one_sprite_line(p, (-95) & 511);
        if (kind == 0) p->oam[1] ^= 1;
        if (kind == 1) p->oam[1] ^= 0x2000;
        if (kind == 2) p->oam[0] ^= 0x100;
        if (kind == 3) p->highOam[0] ^= 2;
        PpuUpdateWidescreenOamHistory(p, 0);
        assert(p->wsOamMotionGrace[0] == 0);
    }
    /* No coordinate blacklist: raw320 remains eligible with real small steps. */
    prime(p, -190);
    field(p, 320);
    assert(p->wsOamMotionGrace[0] == 4 && pixels[Extra - 192] != 0);
    ppu_free(p);
    printf("PASS %u signed motion boundaries, stopped grace, four ownership changes, repeat-line guard and genuine X320 movement\n", boundaries);
    return 0;
}
