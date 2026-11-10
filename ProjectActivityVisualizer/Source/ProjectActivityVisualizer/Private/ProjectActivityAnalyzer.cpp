#include "ProjectActivityAnalyzer.h"
#include "ProjectActivitySettings.h"

TArray<FProjectActivityCategoryStats>
FProjectActivityAnalyzer::SummarizeCategories(const TArray<FProjectActivityBucketStats>& PackageStats, const UProjectActivitySettings& Settings)
{
    TMap<FName, FProjectActivityCategoryStats> CategoryTotals;

    for (const FProjectActivityBucketStats& Package : PackageStats)
    {
        const FName CategoryName = Settings.ResolveCategory(Package.PackagePath);

        FProjectActivityCategoryStats& Total = CategoryTotals.FindOrAdd(CategoryName);

        Total.CategoryName = CategoryName;
        Total.SaveCount += Package.RawSaveCount;
        Total.ActivityBucketCount += Package.ActivityBucketCount;
    }

    TArray<FProjectActivityCategoryStats> Results;
    Results.Reserve(CategoryTotals.Num());

    for (const TPair<FName, FProjectActivityCategoryStats>& Entry : CategoryTotals)
    {
        Results.Add(Entry.Value);
    }

    Results.Sort(
        [](const FProjectActivityCategoryStats& A, const FProjectActivityCategoryStats& B)
        {
            if (A.ActivityBucketCount != B.ActivityBucketCount)
            {
                return A.ActivityBucketCount > B.ActivityBucketCount;
            }

            if (A.SaveCount != B.SaveCount)
            {
                return A.SaveCount > B.SaveCount;
            }

            return A.CategoryName.LexicalLess(B.CategoryName);
        });

    return Results;
}

TArray<FProjectActivityHotspotCandidate>
FProjectActivityAnalyzer::FindHotspotCandidates(const TArray<FProjectActivityHotspotEvidence>& EvidenceRows, const UProjectActivitySettings& Settings)
{
    const int32 MinBuckets =
        FMath::Max(1, Settings.HotspotMinActivityBuckets);

    const int32 MinAliases =
        FMath::Max(2, Settings.HotspotMinNamedAliases);

    const int32 MinReferencers =
        FMath::Max(1, Settings.HotspotMinReferencers);

    TArray<FProjectActivityHotspotCandidate> Candidates;

    for (const FProjectActivityHotspotEvidence& Evidence
        : EvidenceRows)
    {
        if (Evidence.ActivityBucketCount < MinBuckets)
        {
            continue;
        }

        FProjectActivityHotspotCandidate Candidate;
        Candidate.Evidence = Evidence;

        Candidate.bMultipleAliasActivity =
            Evidence.NamedAliasCount >= MinAliases;

        Candidate.bReferencedActivity =
            Evidence.GameReferencerCount >= MinReferencers;

        if (Candidate.bMultipleAliasActivity
            || Candidate.bReferencedActivity)
        {
            Candidates.Add(MoveTemp(Candidate));
        }
    }

    return Candidates;
}