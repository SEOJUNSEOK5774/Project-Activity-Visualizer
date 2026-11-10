#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProjectActivityUserSettings.generated.h"

// 현재 프로젝트에서 사용할 사용자별 활동 설정
UCLASS(Config = EditorPerProjectUserSettings,
    meta = (DisplayName = "Project Activity"))
    class PROJECTACTIVITYVISUALIZER_API UProjectActivityUserSettings
    : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetContainerName() const override;

    virtual FName GetCategoryName() const override;

    virtual FName GetSectionName() const override;

    UPROPERTY(Config , EditAnywhere , Category = "Contributor" , meta = (DisplayName = "Contributor Alias" , ToolTip = "Alias to use for new activity records."))
    FString ContributorAlias;

    UPROPERTY(Config, EditAnywhere, Category = "Team Sharing", meta = ( DisplayName = "Team Exchange Folder", ToolTip = "Absolute local path to the folder used to exchange activity JSON files."))
    FString TeamExchangeFolder;

    UPROPERTY(Config, EditAnywhere, Category = "Team Sharing", meta = (DisplayName = "Enable Automatic Import", ToolTip = "Check the team exchange folder every 5 seconds while this editor is running."))

    bool bEnableAutomaticImport = false;
};
