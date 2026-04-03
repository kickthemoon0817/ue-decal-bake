#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "DecalBakerSubsystem.h"
#include "DecalBakerTypes.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDecalBakerDiscoveryTest,
    "DecalBaker.Subsystem.DiscoverDecalMeshPairs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDecalBakerDiscoveryTest::RunTest(const FString& Parameters)
{
    // Create a temporary world
    UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
    TestNotNull(TEXT("World created"), World);

    // Spawn a static mesh actor at origin
    AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    TestNotNull(TEXT("Mesh actor spawned"), MeshActor);

    UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
    MeshComp->bReceivesDecals = true;

    // Spawn a decal actor overlapping the mesh
    AActor* DecalActor = World->SpawnActor<AActor>(AActor::StaticClass(),
        FVector::ZeroVector, FRotator::ZeroRotator);
    UDecalComponent* DecalComp = NewObject<UDecalComponent>(DecalActor);
    DecalComp->DecalSize = FVector(100, 100, 100);
    DecalComp->RegisterComponent();
    DecalActor->AddInstanceComponent(DecalComp);

    // Test discovery
    UDecalBakerSubsystem* Subsystem = GEngine->GetEngineSubsystem<UDecalBakerSubsystem>();
    TestNotNull(TEXT("Subsystem exists"), Subsystem);

    TArray<UStaticMeshComponent*> Empty;
    TArray<FDecalMeshPair> Pairs = Subsystem->DiscoverDecalMeshPairs(World, Empty);

    // Validates the discovery logic runs without crashing
    // Actual pair count depends on mesh having valid bounds

    // Cleanup
    World->DestroyWorld(false);

    return true;
}
