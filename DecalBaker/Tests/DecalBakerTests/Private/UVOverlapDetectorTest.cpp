#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UVOverlapDetector.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUVOverlapNonOverlappingTest,
    "DecalBaker.UVOverlap.NonOverlappingTriangles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUVOverlapNonOverlappingTest::RunTest(const FString& Parameters)
{
    TArray<FVector2D> UVs = {
        FVector2D(0, 0), FVector2D(0.5, 0), FVector2D(0, 0.5),
        FVector2D(0.5, 0.5), FVector2D(1, 0.5), FVector2D(0.5, 1)
    };
    TArray<uint32> Indices = { 0, 1, 2, 3, 4, 5 };

    bool bResult = FUVOverlapDetector::HasOverlappingTriangles(UVs, Indices);
    TestFalse(TEXT("Non-overlapping triangles should return false"), bResult);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUVOverlapOverlappingTest,
    "DecalBaker.UVOverlap.OverlappingTriangles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUVOverlapOverlappingTest::RunTest(const FString& Parameters)
{
    TArray<FVector2D> UVs = {
        FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1),
        FVector2D(0.25, 0.25), FVector2D(0.75, 0.25), FVector2D(0.25, 0.75)
    };
    TArray<uint32> Indices = { 0, 1, 2, 3, 4, 5 };

    bool bResult = FUVOverlapDetector::HasOverlappingTriangles(UVs, Indices);
    TestTrue(TEXT("Overlapping triangles should return true"), bResult);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUVOverlapAdjacentTest,
    "DecalBaker.UVOverlap.AdjacentTrianglesNotOverlapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUVOverlapAdjacentTest::RunTest(const FString& Parameters)
{
    TArray<FVector2D> UVs = {
        FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1),
        FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1)
    };
    TArray<uint32> Indices = { 0, 1, 2, 3, 4, 5 };

    bool bResult = FUVOverlapDetector::HasOverlappingTriangles(UVs, Indices);
    TestFalse(TEXT("Adjacent triangles sharing edge should not be flagged"), bResult);
    return true;
}
