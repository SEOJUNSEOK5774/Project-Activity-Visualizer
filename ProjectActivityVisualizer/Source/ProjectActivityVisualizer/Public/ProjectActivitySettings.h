#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProjectActivitySettings.generated.h"

// 폴더 경로와 카테고리를 연결하는 규칙 하나
USTRUCT()
struct PROJECTACTIVITYVISUALIZER_API FProjectActivityCategoryRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Category Rule")
    FString PathPrefix;

    UPROPERTY(EditAnywhere, Category = "Category Rule")
    FName CategoryName = NAME_None;
};

// 프로젝트별 활동 분석 설정
UCLASS(Config = Editor , DefaultConfig , meta = (DisplayName = "Project Activity"))

class PROJECTACTIVITYVISUALIZER_API UProjectActivitySettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:

    UPROPERTY(Config, EditAnywhere, Category = "Collection", meta = ( DisplayName = "Enable Activity Collection", ToolTip = "Record new asset save events. Disabling this keeps existing records available."))
    bool bEnableActivityCollection = true;

    UPROPERTY(Config, EditAnywhere, Category = "Collection", meta = (DisplayName = "Tracked Paths", ToolTip = "Collect save events from these /Game/ folders. An empty list collects nothing."))
    TArray<FString> TrackedPaths = { TEXT("/Game/") };

    UPROPERTY(Config, EditAnywhere, Category = "Collection", meta = (DisplayName = "Ignored Paths",ToolTip = "Exclude these /Game/ folders. Ignored paths take priority."))
    TArray<FString> IgnoredPaths;

    // 선택한 기간에서 활동 표시를 붙일 최소 저장 횟수
    UPROPERTY(Config, EditAnywhere, Category = "Analysis", meta = (DisplayName = "High Save Count Threshold", ClampMin = "1", UIMin = "1", ToolTip = "Minimum save-event count in the selected time range."))
    int32 HotAssetMinSaveCount = 10;

    UPROPERTY(Config, EditAnywhere, Category = "Hotspot", meta = (DisplayName = "Minimum Activity Buckets", ClampMin = "1", UIMin = "1", ToolTip = "Minimum 5-minute activity buckets required for a review candidate."))
    int32 HotspotMinActivityBuckets = 3;

    UPROPERTY(Config, EditAnywhere, Category = "Hotspot", meta = (DisplayName = "Minimum Named Aliases", ClampMin = "2", UIMin = "2", ToolTip = "Minimum named aliases for the multiple-alias activity condition."))
    int32 HotspotMinNamedAliases = 2;

    UPROPERTY(Config, EditAnywhere, Category = "Hotspot", meta = (DisplayName = "Minimum Referencers", ClampMin = "1", UIMin = "1", ToolTip = "Minimum current direct /Game/ referencers for the referenced-asset activity condition."))
    int32 HotspotMinReferencers = 1;

    // 패키지 경로가 추적 및 제외 규칙을 통과하는지 확인한다.
    bool ShouldTrackPackage(const FString& PackagePath) const;

    virtual FName GetContainerName() const override;

    virtual FName GetCategoryName() const override;

    virtual FName GetSectionName() const override;

    UPROPERTY(Config,EditAnywhere , Category = "Categories" , meta = (TitleProperty = "CategoryName"))
    TArray<FProjectActivityCategoryRule> CategoryRules;

    FName ResolveCategory(const FString& PackagePath) const;
};
