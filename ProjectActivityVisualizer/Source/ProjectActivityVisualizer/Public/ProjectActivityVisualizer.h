// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ProjectActivityStorage.h"

class FToolBarBuilder;
class FMenuBuilder;
class UPackage;
class FObjectPostSaveContext;
class FReply;
class SDockTab;
class STextBlock;
class SScrollBox;

class FProjectActivityVisualizerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void PluginButtonClicked();

private:
    void RegisterMenus();

    TSharedRef<SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

    void OnPackageSaved(const FString& PackageFileName , UPackage* Package , FObjectPostSaveContext SaveContext);

    // DB를 조회해서 화면의 기록 개수를 갱신한다.
    void RefreshDashboard();

    // 선택한 기간의 카테고리 통계를 화면에 표시한다.
    void RefreshCategoryStats(const FProjectActivityTimeRange& TimeRange);

    // 최근 7개 UTC 날짜의 저장 횟수를 화면에 표시한다.
    void RefreshDailyStats();

    // 새로고침 버튼을 눌렀을 때 호출된다.
    FReply OnRefreshClicked();

    // 기간을 선택하고 대시보드를 갱신한다.
    FReply OnTimeRangeClicked(bool bNewUseLast7Days);

    // 기록된 패키지의 에셋을 Content Browser에서 찾는다.
    FReply OnFindAssetClicked(FString PackagePath);

private:
    TSharedPtr<class FUICommandList> PluginCommands;

    FDelegateHandle PackageSavedDelegateHandle;

    FProjectActivityStorage ActivityStorage;

    // 개수를 표시할 텍스트 위젯
    TSharedPtr<STextBlock> RecordCountText;

    // 패키지별 저장 횟수를 표시할 스크롤 영역
    TSharedPtr<SScrollBox> PackageStatsList;

    // 목록의 조회 상태를 표시할 텍스트
    TSharedPtr<STextBlock> PackageStatsStatusText;

    // 현재 대시보드 탭
    TWeakPtr<SDockTab> DashboardTab;

    // 처음에는 전체 기간을 선택한다.
    bool bUseLast7Days = false;

    // 현재 선택한 기간을 표시한다.
    TSharedPtr<STextBlock> TimeRangeText;

    // 카테고리별 저장 횟수와 비율을 표시할 목록
    TSharedPtr<SScrollBox> CategoryStatsList;

    // 카테고리 조회 결과와 오류를 표시할 텍스트
    TSharedPtr<STextBlock> CategoryStatsStatusText;

    // 날짜별 저장 횟수 그래프를 표시할 목록
    TSharedPtr<SScrollBox> DailyStatsList;

    // 날짜별 조회 상태와 그래프 기준을 표시할 텍스트
    TSharedPtr<STextBlock> DailyStatsStatusText;
};
