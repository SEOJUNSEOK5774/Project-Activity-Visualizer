// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ProjectActivityStorage.h"
#include "Containers/Ticker.h"

class FToolBarBuilder;
class FMenuBuilder;
class UPackage;
class FObjectPostSaveContext;
class FReply;
class SDockTab;
class STextBlock;
class SScrollBox;
class SWindow;
struct FProjectActivitySourceControlQuery;
class SWidgetSwitcher;

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

    // 여러 별칭으로 저장된 패키지를 화면에 표시한다.
    void RefreshMultiContributorStats(const FProjectActivityTimeRange& TimeRange);

    // 활동 통계에 별칭과 현재 참조 관계를 결합한다.
    void BuildHotspotEvidence(const TArray<FProjectActivityBucketStats>& PackageStats, const FProjectActivityTimeRange& TimeRange, TArray<FProjectActivityHotspotEvidence>& OutEvidence);

    // 수집한 근거를 로그로 확인한다.
    void LogHotspotEvidence(const TArray<FProjectActivityHotspotEvidence>& EvidenceRows, const FProjectActivityTimeRange& TimeRange);

    // 원본 저장 횟수와 활동 구간 수를 로그로 확인한다.
    void LogActivityBucketStats(const FProjectActivityTimeRange& TimeRange);

    // 새로고침 버튼을 눌렀을 때 호출된다.
    FReply OnRefreshClicked();

    // 전체 활동 기록을 JSON 파일로 내보낸다.
    FReply OnExportJsonClicked();

    // 설정한 팀 교환 폴더로 전체 기록을 내보낸다.
    FReply OnExportTeamJsonClicked();

    // 선택한 JSON 파일에서 활동 기록을 가져온다.
    FReply OnImportJsonClicked();

    // 기간을 선택하고 대시보드를 갱신한다.
    FReply OnTimeRangeClicked(EProjectActivityTimePreset NewPreset);

    // 선택한 기간을 실제 UTC 시작·종료 시각으로 변환한다.
    FProjectActivityTimeRange MakeSelectedTimeRange() const;

    // 기록된 패키지의 에셋을 Content Browser에서 찾는다.
    FReply OnFindAssetClicked(FString PackagePath);

    // 패키지의 직접 참조 관계를 로그로 표시한다.
    FReply OnShowReferencesClicked(FString PackagePath);

    // 조회한 직접 참조 관계를 상세 창에 표시한다.
    void ShowReferenceDetailsWindow(const FString& PackagePath, const TArray<FName>& Dependencies, const TArray<FName>& Referencers);

    // 패키지의 직접 참조 관계를 조회하고 상세 창을 연다.
    FReply OnShowAliasesClicked(FString PackagePath, FProjectActivityTimeRange TimeRange);

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

    // 현재 열려 있는 별칭 상세 창
    TWeakPtr<SWindow> AliasDetailsWindow;

    // 현재 열려 있는 참조 관계 상세 창
    TWeakPtr<SWindow> ReferenceDetailsWindow;

    EProjectActivityTimePreset SelectedTimePreset = EProjectActivityTimePreset::AllTime;

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

    // 여러 Contributor 별칭이 기록된 패키지 목록
    TSharedPtr<SScrollBox> MultiContributorList;

    // 목록의 조회 결과와 오류를 표시한다.
    TSharedPtr<STextBlock> MultiContributorStatusText;

    // 현재 소스 컨트롤 제공자와 상태를 로그로 출력한다.
    void LogSourceControlStatus();

    // 지정한 패키지의 소스 컨트롤 상태와 이력을 조회한다.
    void QueryPackageSourceControl(const FString& PackagePath);

    // 패키지 목록의 Show History 버튼을 처리한다.
    FReply OnShowHistoryClicked(FString PackagePath);

    // 진행 중인 조회와 완료 콜백의 수명을 관리한다.
    TSharedPtr<FProjectActivitySourceControlQuery> SourceControlQuery;

    struct FAutoImportFileState
    {
        int64 Size = -1;
        FDateTime ModifiedTime = FDateTime(0);
        bool bImported = false;
        double NextRetryTime = 0.0;
    };

    bool TickAutoImport(float DeltaTime);

    void ResetAutoImportState();

    FTSTicker::FDelegateHandle AutoImportTickerHandle;

    FString AutoImportFolder;

    TMap<FString, FAutoImportFileState> AutoImportFiles;

    bool bAutoImportFolderWarningShown = false;

    // Hotspot 후보와 선정 이유를 화면에 표시한다.
    void RefreshHotspotCandidates(const TArray<FProjectActivityHotspotEvidence>& EvidenceRows, const FProjectActivityTimeRange& TimeRange);

    TSharedPtr<STextBlock> HotspotStatusText;

    TSharedPtr<STextBlock> HotspotDetailsText;

    // 대시보드의 표시 페이지를 변경한다.
    FReply OnDashboardPageClicked(int32 PageIndex);

    // 0: Activity, 1: Collaboration / Hotspot, 2: Commit history
    TSharedPtr<SWidgetSwitcher> DashboardPages;
};
