#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "DecalBakerTypes.h"
#include "Json.h"
#include "JsonObjectConverter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FManifestSerializationTest,
    "DecalBaker.Manifest.SerializeDeserialize",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FManifestSerializationTest::RunTest(const FString& Parameters)
{
    FDecalBakeManifest Manifest;
    Manifest.Version = 1;

    FDecalBakeResult Entry;
    Entry.MeshPath = TEXT("/Game/Meshes/SM_Wall_01");
    Entry.OriginalMaterialPath = TEXT("/Game/Materials/M_Wall_01");
    Entry.BakedMaterialPath = TEXT("/Game/BakedDecals/SM_Wall_01/MI_SM_Wall_01_Baked");
    Entry.UVChannel = 0;
    Entry.Resolution = 2048;
    Entry.DecalActorNames.Add(TEXT("DecalActor_12"));
    Entry.DecalActorNames.Add(TEXT("DecalActor_15"));
    Entry.BakeTime = FDateTime(2024, 1, 1);
    Manifest.Entries.Add(Entry);

    FString JsonString;
    bool bSerializeOk = FJsonObjectConverter::UStructToJsonObjectString(Manifest, JsonString);
    TestTrue(TEXT("Serialization succeeds"), bSerializeOk);
    TestTrue(TEXT("JSON contains mesh path"), JsonString.Contains(TEXT("SM_Wall_01")));

    FDecalBakeManifest Loaded;
    bool bDeserializeOk = FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Loaded);
    TestTrue(TEXT("Deserialization succeeds"), bDeserializeOk);
    TestEqual(TEXT("Version matches"), Loaded.Version, 1);
    TestEqual(TEXT("Entry count matches"), Loaded.Entries.Num(), 1);
    TestEqual(TEXT("Mesh path matches"), Loaded.Entries[0].MeshPath, Entry.MeshPath);
    TestEqual(TEXT("Resolution matches"), Loaded.Entries[0].Resolution, 2048);
    TestEqual(TEXT("UVChannel matches"), Loaded.Entries[0].UVChannel, 0);
    TestEqual(TEXT("OriginalMaterialPath matches"),
        Loaded.Entries[0].OriginalMaterialPath, Entry.OriginalMaterialPath);
    TestEqual(TEXT("BakedMaterialPath matches"),
        Loaded.Entries[0].BakedMaterialPath, Entry.BakedMaterialPath);
    TestEqual(TEXT("BakeTime matches"), Loaded.Entries[0].BakeTime, FDateTime(2024, 1, 1));
    TestEqual(TEXT("Decal count matches"), Loaded.Entries[0].DecalActorNames.Num(), 2);
    TestEqual(TEXT("First decal name matches"),
        Loaded.Entries[0].DecalActorNames[0], FString(TEXT("DecalActor_12")));
    TestEqual(TEXT("Second decal name matches"),
        Loaded.Entries[0].DecalActorNames[1], FString(TEXT("DecalActor_15")));

    return true;
}
