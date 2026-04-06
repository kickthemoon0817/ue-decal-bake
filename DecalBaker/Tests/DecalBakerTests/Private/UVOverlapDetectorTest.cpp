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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUVOverlapDegenerateTriangleTest,
    "DecalBaker.UVOverlap.DegenerateZeroAreaTriangle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUVOverlapDegenerateTriangleTest::RunTest(const FString& Parameters)
{
    // A degenerate (zero-area) triangle where all three vertices are collinear
    // paired with a valid triangle should not report overlap.
    TArray<FVector2D> UVs = {
        FVector2D(0, 0), FVector2D(0.5, 0), FVector2D(1, 0),       // degenerate: collinear
        FVector2D(0.5, 0.5), FVector2D(1, 0.5), FVector2D(0.5, 1)  // valid triangle
    };
    TArray<uint32> Indices = { 0, 1, 2, 3, 4, 5 };

    bool bResult = FUVOverlapDetector::HasOverlappingTriangles(UVs, Indices);
    TestFalse(TEXT("Degenerate zero-area triangle should not cause false overlap"), bResult);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUVOverlapEmptyInputTest,
    "DecalBaker.UVOverlap.EmptyInputArrays",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUVOverlapEmptyInputTest::RunTest(const FString& Parameters)
{
    // Empty UV and index arrays should not crash and should return false.
    TArray<FVector2D> EmptyUVs;
    TArray<uint32> EmptyIndices;

    bool bResult = FUVOverlapDetector::HasOverlappingTriangles(EmptyUVs, EmptyIndices);
    TestFalse(TEXT("Empty input should return false"), bResult);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUVOverlapOutOfRangeTest,
    "DecalBaker.UVOverlap.UVsOutsideZeroOneRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUVOverlapOutOfRangeTest::RunTest(const FString& Parameters)
{
    // UV coordinates outside [0,1] range. Two non-overlapping triangles that
    // happen to have coordinates beyond the standard UV space.
    TArray<FVector2D> UVs = {
        FVector2D(-1.0, -1.0), FVector2D(2.0, -1.0), FVector2D(-1.0, 2.0),
        FVector2D(3.0, 3.0),   FVector2D(5.0, 3.0),  FVector2D(3.0, 5.0)
    };
    TArray<uint32> Indices = { 0, 1, 2, 3, 4, 5 };

    bool bResult = FUVOverlapDetector::HasOverlappingTriangles(UVs, Indices);
    TestFalse(TEXT("Non-overlapping out-of-range UVs should return false"), bResult);

    // Now test overlapping triangles with out-of-range coordinates
    TArray<FVector2D> OverlappingUVs = {
        FVector2D(-1.0, -1.0), FVector2D(2.0, -1.0), FVector2D(-1.0, 2.0),
        FVector2D(-0.5, -0.5), FVector2D(1.0, -0.5), FVector2D(-0.5, 1.0)
    };

    bool bOverlapResult = FUVOverlapDetector::HasOverlappingTriangles(OverlappingUVs, Indices);
    TestTrue(TEXT("Overlapping out-of-range UVs should return true"), bOverlapResult);

    return true;
}
