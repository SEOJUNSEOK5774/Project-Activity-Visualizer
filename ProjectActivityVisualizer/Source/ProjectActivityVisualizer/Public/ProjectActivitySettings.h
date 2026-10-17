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
    virtual FName GetContainerName() const override;

    virtual FName GetCategoryName() const override;

    virtual FName GetSectionName() const override;

    UPROPERTY(Config,EditAnywhere , Category = "Categories" , meta = (TitleProperty = "CategoryName"))
    TArray<FProjectActivityCategoryRule> CategoryRules;

    FName ResolveCategory(const FString& PackagePath) const;
};
