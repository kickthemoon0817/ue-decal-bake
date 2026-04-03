#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "DecalProjection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDecalProjectionMatrixTest,
    "DecalBaker.DecalProjection.ComputeProjectionMatrix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDecalProjectionMatrixTest::RunTest(const FString& Parameters)
{
    FTransform DecalTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector);
    FVector DecalSize(100.0f, 100.0f, 100.0f);

    FMatrix ProjMatrix = FDecalProjection::ComputeProjectionMatrix(DecalTransform, DecalSize);

    FVector4 Origin = ProjMatrix.TransformFVector4(FVector4(0, 0, 0, 1));
    TestNearlyEqual(TEXT("Origin maps to center"), FVector(Origin), FVector::ZeroVector, 0.001f);

    FVector4 Edge = ProjMatrix.TransformFVector4(FVector4(0, 100, 0, 1));
    TestNearlyEqual(TEXT("Edge maps to 1.0"), Edge.Y, 1.0f, 0.001f);

    FVector4 Outside = ProjMatrix.TransformFVector4(FVector4(0, 200, 0, 1));
    TestTrue(TEXT("Outside point > 1.0"), FMath::Abs(Outside.Y) > 1.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDecalProjectionIsInsideTest,
    "DecalBaker.DecalProjection.IsPointInsideDecal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDecalProjectionIsInsideTest::RunTest(const FString& Parameters)
{
    FTransform DecalTransform(FRotator::ZeroRotator, FVector(500, 0, 0), FVector::OneVector);
    FVector DecalSize(100.0f, 200.0f, 150.0f);

    FMatrix ProjMatrix = FDecalProjection::ComputeProjectionMatrix(DecalTransform, DecalSize);

    TestTrue(TEXT("Center is inside"),
        FDecalProjection::IsPointInsideDecal(FVector(500, 0, 0), ProjMatrix));

    TestFalse(TEXT("Far point is outside"),
        FDecalProjection::IsPointInsideDecal(FVector(0, 0, 0), ProjMatrix));

    return true;
}
