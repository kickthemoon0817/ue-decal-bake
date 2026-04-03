#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "DecalBakerCommandlet.generated.h"

UCLASS()
class UDecalBakerCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UDecalBakerCommandlet();
    virtual int32 Main(const FString& Params) override;
};
