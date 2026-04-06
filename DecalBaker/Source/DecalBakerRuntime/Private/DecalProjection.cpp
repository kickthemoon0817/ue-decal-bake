#include "DecalProjection.h"
#include "Components/DecalComponent.h"

FMatrix FDecalProjection::ComputeProjectionMatrix(const FTransform& DecalTransform, const FVector& DecalSize)
{
    FMatrix WorldToLocal = DecalTransform.ToInverseMatrixWithScale();

    FMatrix SizeNormalize = FScaleMatrix(FVector(
        DecalSize.X > 0 ? 1.0f / DecalSize.X : 0.0f,
        DecalSize.Y > 0 ? 1.0f / DecalSize.Y : 0.0f,
        DecalSize.Z > 0 ? 1.0f / DecalSize.Z : 0.0f
    ));

    return WorldToLocal * SizeNormalize;
}

FMatrix FDecalProjection::ComputeProjectionMatrix(const UDecalComponent* DecalComponent)
{
    check(DecalComponent);
    return ComputeProjectionMatrix(
        DecalComponent->GetComponentTransform(),
        DecalComponent->DecalSize
    );
}

bool FDecalProjection::IsPointInsideDecal(const FVector& WorldPoint, const FMatrix& ProjectionMatrix)
{
    FVector4 LocalPos = ProjectionMatrix.TransformFVector4(FVector4(WorldPoint, 1.0f));
    return FMath::Abs(LocalPos.X) <= 1.0f
        && FMath::Abs(LocalPos.Y) <= 1.0f
        && FMath::Abs(LocalPos.Z) <= 1.0f;
}

FVector FDecalProjection::GetDecalForwardDir(const FTransform& DecalTransform)
{
    return -DecalTransform.GetUnitAxis(EAxis::X);
}
