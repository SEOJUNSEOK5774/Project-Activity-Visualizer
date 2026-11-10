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

bool UProjectActivitySettings::ShouldTrackPackage(const FString& PackagePath) const
{
    // 현재 수집 범위는 프로젝트 Content 아래로 제한한다.
    if (!PackagePath.StartsWith(TEXT("/Game/"), ESearchCase::IgnoreCase))
    {
        return false;
    }

    const auto MatchesFolder = [&PackagePath](const FString& RulePath)
        {
            FString Prefix = RulePath.TrimStartAndEnd();

            if (Prefix.IsEmpty())
            {
                return false;
            }

            Prefix.ReplaceInline(TEXT("\\"), TEXT("/"));

            if (!Prefix.EndsWith(TEXT("/")))
            {
                Prefix += TEXT("/");
            }

            if (!Prefix.StartsWith(TEXT("/Game/"), ESearchCase::IgnoreCase))
            {
                return false;
            }

            return PackagePath.StartsWith(Prefix, ESearchCase::IgnoreCase);
        };

    // 제외 규칙이 우선한다.
    for (const FString& IgnoredPath : IgnoredPaths)
    {
        if (MatchesFolder(IgnoredPath))
        {
            return false;
        }
    }

    // 추적 규칙 중 하나에 해당하면 수집한다.
    for (const FString& TrackedPath : TrackedPaths)
    {
        if (MatchesFolder(TrackedPath))
        {
            return true;
        }
    }

    return false;
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