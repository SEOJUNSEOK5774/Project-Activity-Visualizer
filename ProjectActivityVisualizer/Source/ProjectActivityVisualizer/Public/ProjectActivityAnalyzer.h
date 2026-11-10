#pragma once

#include "CoreMinimal.h"
#include "ProjectActivityTypes.h"

class UProjectActivitySettings;

class PROJECTACTIVITYVISUALIZER_API FProjectActivityAnalyzer
{
public:
    static TArray<FProjectActivityCategoryStats> SummarizeCategories(const TArray<FProjectActivityBucketStats>& PackageStats, const UProjectActivitySettings& Settings);

    static TArray<FProjectActivityHotspotCandidate> FindHotspotCandidates(const TArray<FProjectActivityHotspotEvidence>& EvidenceRows, const UProjectActivitySettings& Settings);
};
