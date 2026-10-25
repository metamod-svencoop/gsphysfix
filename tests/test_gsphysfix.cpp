#include "physfix.h"

#include <cstring>
#include <iostream>
#include <memory>

#define CHECK(condition)                                   \
    do                                                     \
    {                                                      \
        if (!(condition))                                  \
        {                                                  \
            std::cerr << __LINE__ << ": " #condition "\n"; \
            return 1;                                      \
        }                                                  \
    } while (0)

enum
{
    TestModel_None,
    TestModel_StudioHitbox,
    TestModel_StudioBox,
    TestModel_BrushHitbox,
    TestModel_SpriteHitbox,
    TestModel_Empty,
    TestModel_Count
};

static model_t  g_Models[TestModel_Count];
static model_t* g_pModels[TestModel_Count];

static const int TEST_EDICT_COUNT = 8;
static edict_t   g_Edicts[TEST_EDICT_COUNT];

static edict_t* TestEdictOfIndex(int iEntIndex)
{
    return (iEntIndex >= 0 && iEntIndex < TEST_EDICT_COUNT) ? &g_Edicts[iEntIndex] : NULL;
}

static void InitModels()
{
    memset(g_Models, 0, sizeof(g_Models));

    g_Models[TestModel_StudioHitbox].type  = mod_studio;
    g_Models[TestModel_StudioHitbox].flags = FMODEL_TRACE_HITBOX;
    g_Models[TestModel_StudioBox].type     = mod_studio;
    g_Models[TestModel_BrushHitbox].type   = mod_brush;
    g_Models[TestModel_BrushHitbox].flags  = FMODEL_TRACE_HITBOX;
    g_Models[TestModel_SpriteHitbox].type  = mod_sprite;
    g_Models[TestModel_SpriteHitbox].flags = FMODEL_TRACE_HITBOX;

    for (int i = 0; i < TestModel_Count; i++)
        g_pModels[i] = (i == TestModel_None || i == TestModel_Empty) ? NULL : &g_Models[i];
}

static edict_t MakeEdict(int solid, int modelindex)
{
    edict_t edict      = {};
    edict.v.solid      = solid;
    edict.v.modelindex = modelindex;
    return edict;
}

static physent_t MakePhysent(int info, int solid)
{
    physent_t pe = {};
    pe.info      = info;
    pe.solid     = solid;
    return pe;
}

// Appends the converted entity e to the visents and, as the walker does, to physents.
template <typename TPlayerMove>
static void AppendEntity(TPlayerMove* pm, int e)
{
    physent_t pe = MakePhysent(e, g_Edicts[e].v.solid);

    pm->visents[pm->numvisent++]   = pe;
    pm->physents[pm->numphysent++] = pe;
}

template <typename TPlayerMove>
static int TestAddLinksToPM()
{
    auto pm = std::make_unique<TPlayerMove>();

    for (int i = 0; i < TEST_EDICT_COUNT; i++)
        g_Edicts[i] = MakeEdict(SOLID_SLIDEBOX, TestModel_StudioHitbox);

    g_Edicts[2] = MakeEdict(SOLID_SLIDEBOX, TestModel_StudioBox);
    g_Edicts[3] = MakeEdict(SOLID_BBOX, TestModel_StudioHitbox);

    // An entry present before the walk belongs to the caller.
    pm->physents[0] = MakePhysent(5, SOLID_SLIDEBOX);
    pm->visents[0]  = pm->physents[0];
    pm->numphysent  = 1;
    pm->numvisent   = 1;

    int  depth         = 0;
    bool nestedChecked = false;

    // The outer walk adds 1, recurses into a child that adds 2 and 3, then adds 4.
    GSPhysFix_RunAddLinksToPM(pm.get(), depth, g_pModels, TestEdictOfIndex, [&]() {
        AppendEntity(pm.get(), 1);

        GSPhysFix_RunAddLinksToPM(pm.get(), depth, g_pModels, TestEdictOfIndex, [&]() {
            AppendEntity(pm.get(), 2);
            AppendEntity(pm.get(), 3);
        });

        // The child call must not fix entries before the outermost walk finishes.
        nestedChecked = depth == 1 && pm->physents[1].studiomodel == NULL;

        AppendEntity(pm.get(), 4);
    });

    CHECK(nestedChecked);
    CHECK(depth == 0);
    CHECK(pm->numphysent == 5);
    CHECK(pm->numvisent == 5);

    CHECK(pm->physents[0].studiomodel == NULL);
    CHECK(pm->visents[0].studiomodel == NULL);

    for (int i = 1; i < 5; i++)
    {
        model_t* expected = (i == 1 || i == 4) ? &g_Models[TestModel_StudioHitbox] : NULL;

        CHECK(pm->physents[i].studiomodel == expected);
        CHECK(pm->visents[i].studiomodel == expected);
    }

    // A missing playermove passes straight through.
    bool called = false;
    GSPhysFix_RunAddLinksToPM((TPlayerMove*)NULL, depth, g_pModels, TestEdictOfIndex, [&]() { called = true; });
    CHECK(called);
    CHECK(depth == 0);

    // Counts beyond the list capacity are clamped.
    const int capacity = int(std::size(pm->physents));

    pm->numphysent             = capacity - 1;
    pm->numvisent              = capacity - 1;
    pm->physents[capacity - 1] = {};
    pm->visents[capacity - 1]  = {};

    GSPhysFix_RunAddLinksToPM(pm.get(), depth, g_pModels, TestEdictOfIndex, [&]() {
        pm->physents[capacity - 1] = MakePhysent(1, SOLID_SLIDEBOX);
        pm->visents[capacity - 1]  = pm->physents[capacity - 1];
        pm->numphysent             = capacity + 1;
        pm->numvisent              = capacity + 1;
    });

    CHECK(pm->physents[capacity - 1].studiomodel == &g_Models[TestModel_StudioHitbox]);
    CHECK(pm->visents[capacity - 1].studiomodel == &g_Models[TestModel_StudioHitbox]);

    return 0;
}

int main()
{
    InitModels();

    // Only SOLID_SLIDEBOX studio models flagged for hitbox traces qualify.

    edict_t check = MakeEdict(SOLID_SLIDEBOX, TestModel_StudioHitbox);
    CHECK(GSPhysFix_GetSlideBoxHitboxModel(&check, g_pModels) == &g_Models[TestModel_StudioHitbox]);

    for (int solid : {SOLID_NOT, SOLID_TRIGGER, SOLID_BBOX, SOLID_BSP})
    {
        check = MakeEdict(solid, TestModel_StudioHitbox);
        CHECK(GSPhysFix_GetSlideBoxHitboxModel(&check, g_pModels) == NULL);
    }

    for (int modelindex : {int(TestModel_None), int(TestModel_StudioBox), int(TestModel_BrushHitbox), int(TestModel_SpriteHitbox), int(TestModel_Empty), -1})
    {
        check = MakeEdict(SOLID_SLIDEBOX, modelindex);
        CHECK(GSPhysFix_GetSlideBoxHitboxModel(&check, g_pModels) == NULL);
    }

    // Slot 0 is never a model, even when the table holds one there.
    g_pModels[TestModel_None] = &g_Models[TestModel_StudioHitbox];
    check                     = MakeEdict(SOLID_SLIDEBOX, TestModel_None);
    CHECK(GSPhysFix_GetSlideBoxHitboxModel(&check, g_pModels) == NULL);
    g_pModels[TestModel_None] = NULL;

    check = MakeEdict(SOLID_SLIDEBOX, TestModel_StudioHitbox);
    CHECK(GSPhysFix_GetSlideBoxHitboxModel(NULL, g_pModels) == NULL);
    CHECK(GSPhysFix_GetSlideBoxHitboxModel(&check, NULL) == NULL);

    // A converted physent only gains studiomodel.

    // memset and memcpy keep the padding bytes comparable.
    physent_t pe;
    memset(&pe, 0, sizeof(pe));
    strcpy(pe.name, "monster_test");
    pe.solid   = SOLID_SLIDEBOX;
    pe.mins[2] = -36;
    pe.maxs[2] = 36;

    physent_t expected;
    memcpy(&expected, &pe, sizeof(pe));
    expected.studiomodel = &g_Models[TestModel_StudioHitbox];

    GSPhysFix_FixPhysent(&pe, &check, g_pModels);
    CHECK(memcmp(&pe, &expected, sizeof(pe)) == 0);

    // The fix does not clear a studiomodel the engine set.
    physent_t bbox;
    memset(&bbox, 0, sizeof(bbox));
    bbox.studiomodel = &g_Models[TestModel_StudioHitbox];

    physent_t bboxCopy;
    memcpy(&bboxCopy, &bbox, sizeof(bbox));

    edict_t bboxCheck = MakeEdict(SOLID_BBOX, TestModel_StudioHitbox);

    GSPhysFix_FixPhysent(&bbox, &bboxCheck, g_pModels);
    CHECK(memcmp(&bbox, &bboxCopy, sizeof(bbox)) == 0);

    GSPhysFix_FixPhysent(NULL, &check, g_pModels);

    // Ranges fix [first, count) and skip other solids and unknown entities.

    for (int i = 0; i < TEST_EDICT_COUNT; i++)
        g_Edicts[i] = MakeEdict(SOLID_SLIDEBOX, TestModel_StudioHitbox);

    physent_t list[6] = {
        MakePhysent(1, SOLID_SLIDEBOX),
        MakePhysent(2, SOLID_SLIDEBOX),
        MakePhysent(3, SOLID_BBOX),
        MakePhysent(TEST_EDICT_COUNT, SOLID_SLIDEBOX),
        MakePhysent(4, SOLID_SLIDEBOX),
        MakePhysent(5, SOLID_SLIDEBOX),
    };

    GSPhysFix_FixPhysents(list, 1, 5, 6, g_pModels, TestEdictOfIndex);

    CHECK(list[0].studiomodel == NULL);
    CHECK(list[1].studiomodel == &g_Models[TestModel_StudioHitbox]);
    CHECK(list[2].studiomodel == NULL);
    CHECK(list[3].studiomodel == NULL);
    CHECK(list[4].studiomodel == &g_Models[TestModel_StudioHitbox]);
    CHECK(list[5].studiomodel == NULL);

    // The edict, not the physent copy, decides the solid type.
    g_Edicts[5] = MakeEdict(SOLID_BBOX, TestModel_StudioHitbox);
    GSPhysFix_FixPhysents(list, 5, 6, 6, g_pModels, TestEdictOfIndex);
    CHECK(list[5].studiomodel == NULL);

    // Negative starts and counts beyond the capacity are clamped.
    physent_t clamped[2] = {MakePhysent(1, SOLID_SLIDEBOX), MakePhysent(2, SOLID_SLIDEBOX)};
    GSPhysFix_FixPhysents(clamped, -1, 9, 1, g_pModels, TestEdictOfIndex);
    CHECK(clamped[0].studiomodel == &g_Models[TestModel_StudioHitbox]);
    CHECK(clamped[1].studiomodel == NULL);

    GSPhysFix_FixPhysents(NULL, 0, 1, 1, g_pModels, TestEdictOfIndex);
    GSPhysFix_FixPhysents(clamped, 0, 1, 1, g_pModels, NULL);

    // The area walker fix on both playermove layouts.

    if (TestAddLinksToPM<playermove_t>())
        return 1;

    if (TestAddLinksToPM<playermove_10152_t>())
        return 1;

    std::cout << "GSPhysFix behavior tests passed\n";
    return 0;
}
