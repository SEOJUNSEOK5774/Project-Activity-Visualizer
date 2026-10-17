#pragma once

#include "CoreMinimal.h"
#include "ProjectActivityTypes.h"

class UProjectActivitySettings;

class PROJECTACTIVITYVISUALIZER_API FProjectActivityAnalyzer
{
public:
    static TArray<FProjectActivityCategoryStats> SummarizeCategories(const TArray<FProjectActivityPackageStats>& PackageStats, const UProjectActivitySettings& Settings);
};
