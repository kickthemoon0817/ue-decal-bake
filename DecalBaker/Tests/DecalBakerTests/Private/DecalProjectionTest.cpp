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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDecalProjectionKnownTransformTest,
    "DecalBaker.DecalProjection.KnownTransformValues",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDecalProjectionKnownTransformTest::RunTest(const FString& Parameters)
{
    // Verify the transform+size overload produces expected values with known inputs.
    // A decal at (200, 300, 0) with size (50, 75, 100) should map its center
    // to the origin in projection space and scale edges to +/-1.

    const FVector DecalLocation(200.0f, 300.0f, 0.0f);
    const FVector DecalSize(50.0f, 75.0f, 100.0f);
    FTransform DecalTransform(FRotator::ZeroRotator, DecalLocation, FVector::OneVector);

    FMatrix ProjMatrix = FDecalProjection::ComputeProjectionMatrix(DecalTransform, DecalSize);

    // The decal center in world space should map to the origin in projection space
    FVector4 Center = ProjMatrix.TransformFVector4(FVector4(DecalLocation.X, DecalLocation.Y, DecalLocation.Z, 1));
    TestNearlyEqual(TEXT("Center X maps to 0"), Center.X, 0.0f, 0.01f);
    TestNearlyEqual(TEXT("Center Y maps to 0"), Center.Y, 0.0f, 0.01f);
    TestNearlyEqual(TEXT("Center Z maps to 0"), Center.Z, 0.0f, 0.01f);

    // A point at the decal's Y-edge should map to +/-1 in the Y component
    FVector4 YEdge = ProjMatrix.TransformFVector4(
        FVector4(DecalLocation.X, DecalLocation.Y + DecalSize.Y, DecalLocation.Z, 1));
    TestNearlyEqual(TEXT("Y-edge maps to 1.0"), YEdge.Y, 1.0f, 0.01f);

    // The matrix should be invertible (non-degenerate)
    TestTrue(TEXT("Projection matrix is invertible"),
        FMath::Abs(ProjMatrix.Determinant()) > SMALL_NUMBER);

    return true;
}
