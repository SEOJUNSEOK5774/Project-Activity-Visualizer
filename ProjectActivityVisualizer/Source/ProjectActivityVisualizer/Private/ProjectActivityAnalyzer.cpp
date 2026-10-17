#include "ProjectActivityAnalyzer.h"
#include "ProjectActivitySettings.h"

TArray<FProjectActivityCategoryStats>FProjectActivityAnalyzer::SummarizeCategories(
    const TArray<FProjectActivityPackageStats>& PackageStats,
    const UProjectActivitySettings& Settings)
{
    TMap<FName, int64> CategoryCounts;

    for (const FProjectActivityPackageStats& Package : PackageStats)
    {
        const FName CategoryName = Settings.ResolveCategory(Package.PackagePath);

        CategoryCounts.FindOrAdd(CategoryName) += Package.SaveCount;
    }

    TArray<FProjectActivityCategoryStats> Results;
    Results.Reserve(CategoryCounts.Num());

    for (const TPair<FName, int64>& Entry : CategoryCounts)
    {
        FProjectActivityCategoryStats Stats;
        Stats.CategoryName = Entry.Key;
        Stats.SaveCount = Entry.Value;

        Results.Add(MoveTemp(Stats));
    }

    Results.Sort(
        [](const FProjectActivityCategoryStats& A, const FProjectActivityCategoryStats& B)
        {
            if (A.SaveCount != B.SaveCount)
            {
                return A.SaveCount > B.SaveCount;
            }

            return A.CategoryName.LexicalLess(B.CategoryName);
        });

    return Results;
}