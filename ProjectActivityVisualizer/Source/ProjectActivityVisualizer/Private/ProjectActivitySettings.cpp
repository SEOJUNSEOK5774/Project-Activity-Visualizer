#include "ProjectActivitySettings.h"

FName UProjectActivitySettings::GetContainerName() const
{
    return FName(TEXT("Project"));
}

FName UProjectActivitySettings::GetCategoryName() const
{
    return FName(TEXT("Plugins"));
}

FName UProjectActivitySettings::GetSectionName() const
{
    return FName(TEXT("ProjectActivity"));
}

FName UProjectActivitySettings::ResolveCategory(
    const FString& PackagePath) const
{
    FName BestCategory(TEXT("Other"));
    int32 BestPrefixLength = 0;

    for (const FProjectActivityCategoryRule& Rule : CategoryRules)
    {
        if (Rule.CategoryName.IsNone())
        {
            continue;
        }

        FString Prefix = Rule.PathPrefix.TrimStartAndEnd();

        Prefix.ReplaceInline(TEXT("\\"), TEXT("/"));

        if (Prefix.IsEmpty())
        {
            continue;
        }

        if (!Prefix.EndsWith(TEXT("/")))
        {
            Prefix += TEXT("/");
        }

        if (!Prefix.StartsWith(
            TEXT("/Game/"),
            ESearchCase::IgnoreCase))
        {
            continue;
        }

        if (PackagePath.StartsWith(
            Prefix,
            ESearchCase::IgnoreCase)
            && Prefix.Len() > BestPrefixLength)
        {
            BestPrefixLength = Prefix.Len();
            BestCategory = Rule.CategoryName;
        }
    }

    return BestCategory;
}