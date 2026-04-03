#pragma once

#include "CoreMinimal.h"

class FOmniverseExportHook
{
public:
    static void Register();
    static void Unregister();

private:
    static void OnPreExport();
    static void OnPostExport();

    static FDelegateHandle PreExportHandle;
    static bool bBakedForExport;
};
