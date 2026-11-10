// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectActivityVisualizer.h"
#include "ProjectActivityVisualizerStyle.h"
#include "ProjectActivityVisualizerCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"
#include "ProjectActivityTypes.h"
#include "Input/Reply.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/Timespan.h"
#include "ProjectActivitySettings.h"
#include "UObject/UObjectGlobals.h"
#include "ProjectActivityAnalyzer.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Internationalization/Text.h"
#include "ProjectActivityUserSettings.h"
#include "Misc/Paths.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "HAL/FileManager.h"
#include "Misc/ScopeExit.h"
#include "HAL/PlatformTime.h"
#include "CoreGlobals.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ISourceControlState.h"
#include "SourceControlOperations.h"
#include "Misc/PackageName.h"
#include "ISourceControlRevision.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SWrapBox.h"


DEFINE_LOG_CATEGORY_STATIC(LogProjectActivity, Log, All);

struct FProjectActivitySourceControlQuery
{
	FString PackagePath;
	FString FileName;
	FName ProviderName = NAME_None;

	FString RepositoryRoot;
	FString BranchName;

	// 조회 객체는 모듈 종료 시 먼저 해제된다.
	FProjectActivityStorage* Storage = nullptr;

	bool bPending = false;

	FText DisplayText = NSLOCTEXT("ProjectActivitySourceControl", "SelectAssetHistoryMessage", "Click Show History beside a package to view its commit history.");

	bool TryShowCachedSnapshot(const FString& RequestedPackagePath, const FString& Reason)
	{
		if (Storage == nullptr)
		{
			return false;
		}

		FProjectActivitySourceControlSnapshot Cached;
		bool bFound = false;

		if (!Storage->LoadSourceControlSnapshot(RequestedPackagePath, Cached, bFound) || !bFound)
		{
			return false;
		}

		// 다시 Refresh를 누르면 이 패키지를 조회한다.
		PackagePath = Cached.PackagePath;

		FString Summary =
			TEXT("CACHED RESULT - previous lookup.\n")
			TEXT("Click Refresh to check the current state.\n");

		if (!Reason.IsEmpty())
		{
			Summary += Reason + TEXT("\n");
		}

		Summary += FString::Printf(
			TEXT("\nPackage: %s\n")
			TEXT("Provider at lookup: %s\n")
			TEXT("Repository at lookup: %s\n")
			TEXT("Branch at lookup: %s\n")
			TEXT("Fetched UTC: %s\n")
			TEXT("State at lookup: Tracked=%d | Modified=%d | Added=%d | Conflicted=%d\n")
			TEXT("History at lookup: %d | Cached entries: %d\n"),
			*Cached.PackagePath,
			*Cached.ProviderName,
			*Cached.RepositoryRoot,
			*Cached.BranchName,
			*Cached.FetchedAtUtc.ToIso8601(),
			static_cast<int32>(Cached.bTracked),
			static_cast<int32>(Cached.bModified),
			static_cast<int32>(Cached.bAdded),
			static_cast<int32>(Cached.bConflicted),
			Cached.RetrievedHistoryCount,
			Cached.RecentRevisions.Num());

		if (Cached.RecentRevisions.IsEmpty())
		{
			Summary += TEXT("\nNo cached history entries.\n");
		}

		for (const FProjectActivitySourceControlRevision& Revision
			: Cached.RecentRevisions)
		{
			FString Description = Revision.Description;

			Description.ReplaceInline(TEXT("\r"), TEXT(" "));
			Description.ReplaceInline(TEXT("\n"), TEXT(" "));
			Description.ReplaceInline(TEXT("\t"), TEXT(" "));
			Description.TrimStartAndEndInline();

			Summary += FString::Printf(
				TEXT("\nRevision: %s | Author: %s\n")
				TEXT("UTC: %s\n")
				TEXT("%s\n"),
				*Revision.RevisionDisplayId,
				*Revision.AuthorName,
				*Revision.RevisionTimeUtc.ToIso8601(),
				*Description);
		}

		DisplayText = FText::FromString(Summary);

		return true;
	}

	void SetFailure(const FString& Message)
	{
		UE_LOG(LogProjectActivity, Warning, TEXT("%s | Package=%s"), *Message, *PackagePath);

		if (!PackagePath.IsEmpty() && TryShowCachedSnapshot(PackagePath, Message))
		{
			return;
		}

		DisplayText = FText::FromString(FString::Printf(TEXT("Package: %s\n%s\nNo saved local copy is available."), *PackagePath, *Message));
	}

	FString CacheSnapshotAndGetStatus(const FSourceControlStatePtr& State)
	{
		const FString FailureStatus = TEXT("\n\nHistory loaded, but the local copy could not be updated. See Output Log.");

		if (Storage == nullptr || !State.IsValid())
		{
			UE_LOG(LogProjectActivity, Warning, TEXT("Cannot cache source control snapshot: storage or state is unavailable."));

			return FailureStatus;
		}

		FProjectActivitySourceControlSnapshot Snapshot;

		Snapshot.PackagePath = PackagePath;
		Snapshot.ProviderName = ProviderName.ToString();
		Snapshot.RepositoryRoot = RepositoryRoot;
		Snapshot.BranchName = BranchName;
		Snapshot.FetchedAtUtc = FDateTime::UtcNow();

		Snapshot.bTracked = State->IsSourceControlled();
		Snapshot.bModified = State->IsModified();
		Snapshot.bAdded = State->IsAdded();
		Snapshot.bConflicted = State->IsConflicted();

		Snapshot.RetrievedHistoryCount = State->GetHistorySize();

		const int32 CacheCount = FMath::Min(Snapshot.RetrievedHistoryCount, 5);

		for (int32 HistoryIndex = 0; HistoryIndex < CacheCount; ++HistoryIndex)
		{
			const TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe>
				Revision = State->GetHistoryItem(HistoryIndex);

			if (!Revision.IsValid())
			{
				UE_LOG(
					LogProjectActivity,
					Warning,
					TEXT("Snapshot was not cached: a history entry is unavailable | Package=%s | Index=%d"),
					*PackagePath,
					HistoryIndex);

				return FailureStatus;
			}

			FProjectActivitySourceControlRevision CachedRevision;

			CachedRevision.RevisionDisplayId = Revision->GetRevision();

			CachedRevision.AuthorName = Revision->GetUserName();

			CachedRevision.RevisionTimeUtc = Revision->GetDate();

			CachedRevision.Description = Revision->GetDescription();

			Snapshot.RecentRevisions.Add(MoveTemp(CachedRevision));
		}

		if (!Storage->SaveSourceControlSnapshot(Snapshot))
		{
			return FailureStatus;
		}

		return FString::Printf(TEXT("\n\nLocal copy updated: %s"), *Snapshot.FetchedAtUtc.ToIso8601());
	}

	void OnCompleted(const FSourceControlOperationRef&, ECommandResult::Type Result)
	{
		bPending = false;

		if (Result != ECommandResult::Succeeded)
		{
			SetFailure(TEXT("Asset source control query failed."));
			return;
		}

		ISourceControlModule& SourceControl = ISourceControlModule::Get();

		ISourceControlProvider& Provider = SourceControl.GetProvider();

		const TMap<ISourceControlProvider::EStatus, FString>CurrentStatus = Provider.GetStatus();

		if (!SourceControl.IsEnabled() || !Provider.IsAvailable() || Provider.GetName() != ProviderName || CurrentStatus.FindRef(ISourceControlProvider::EStatus::Repository) != RepositoryRoot || CurrentStatus.FindRef(ISourceControlProvider::EStatus::Branch) != BranchName)
		{
			SetFailure(TEXT("Source control provider, repository, or branch changed. Click Refresh to retry."));

			return;
		}

		const FSourceControlStatePtr State =Provider.GetState(FileName, EStateCacheUsage::Use);

		if (!State.IsValid() || State->IsUnknown())
		{
			SetFailure(TEXT("Asset source control state is unknown."));
			return;
		}

		UE_LOG(
			LogProjectActivity,
			Log,
			TEXT("Asset source control | Package=%s | Provider=%s | Tracked=%d | Modified=%d | Added=%d | Conflicted=%d"),
			*PackagePath,
			*ProviderName.ToString(),
			static_cast<int32>(State->IsSourceControlled()),
			static_cast<int32>(State->IsModified()),
			static_cast<int32>(State->IsAdded()),
			static_cast<int32>(State->IsConflicted()));

		const int32 HistoryCount = State->GetHistorySize();
		const int32 DisplayCount = FMath::Min(HistoryCount, 5);

		UE_LOG(
			LogProjectActivity,
			Log,
			TEXT("Asset history summary | Package=%s | Retrieved=%d | Showing=%d"),
			*PackagePath,
			HistoryCount,
			DisplayCount);

		FString Summary = FString::Printf(
			TEXT("Package: %s\n")
			TEXT("Provider: %s | Tracked=%d | Modified=%d | Added=%d | Conflicted=%d\n")
			TEXT("History returned: %d | Showing: %d\n"),
			*PackagePath,
			*ProviderName.ToString(),
			static_cast<int32>(State->IsSourceControlled()),
			static_cast<int32>(State->IsModified()),
			static_cast<int32>(State->IsAdded()),
			static_cast<int32>(State->IsConflicted()),
			HistoryCount,
			DisplayCount);

		if (HistoryCount == 0)
		{
			Summary += TEXT("\nNo history was returned for this asset.");

			Summary += CacheSnapshotAndGetStatus(State);
			DisplayText = FText::FromString(Summary);

			UE_LOG(LogProjectActivity, Log, TEXT("No history was returned for this asset."));

			return;
		}

		for (int32 HistoryIndex = 0; HistoryIndex < DisplayCount; ++HistoryIndex)
		{
			const TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe>
				Revision = State->GetHistoryItem(HistoryIndex);

			if (!Revision.IsValid())
			{
				Summary += TEXT("\nHistory entry unavailable.\n");

				UE_LOG(
					LogProjectActivity,
					Warning,
					TEXT("Asset history item is unavailable | Index=%d"),
					HistoryIndex);

				continue;
			}

			FString Description = Revision->GetDescription();

			Description.ReplaceInline(TEXT("\r"), TEXT(" "));
			Description.ReplaceInline(TEXT("\n"), TEXT(" "));
			Description.ReplaceInline(TEXT("\t"), TEXT(" "));
			Description.TrimStartAndEndInline();

			const FString DateUtc = Revision->GetDate().ToIso8601();

			Summary += FString::Printf(
				TEXT("\nRevision: %s | Author: %s\n")
				TEXT("UTC: %s\n")
				TEXT("%s\n"),
				*Revision->GetRevision(),
				*Revision->GetUserName(),
				*DateUtc,
				*Description);

			UE_LOG(
				LogProjectActivity,
				Log,
				TEXT("Asset history | Revision=%s | Author=%s | DateUtc=%s | Message=%s"),
				*Revision->GetRevision(),
				*Revision->GetUserName(),
				*DateUtc,
				*Description);
		}

		Summary += CacheSnapshotAndGetStatus(State);
		DisplayText = FText::FromString(Summary);
	}
};

static const FName ProjectActivityVisualizerTabName("ProjectActivityVisualizer");

#define LOCTEXT_NAMESPACE "FProjectActivityVisualizerModule"

static FText MakeActivityTimeRangeText(const FProjectActivityTimeRange& TimeRange)
{
	if (!TimeRange.bEnabled)
	{
		return LOCTEXT(
			"ActivityRangeAllTime",
			"All time");
	}

	const FTimespan Duration = TimeRange.EndUtc - TimeRange.StartUtc;

	FText WindowName = LOCTEXT("ActivityRangeSelected", "Selected range");

	if (Duration == FTimespan::FromHours(24.0))
	{
		WindowName = LOCTEXT(
			"ActivityRange24Hours",
			"24-hour window");
	}
	else if (Duration == FTimespan::FromDays(7.0))
	{
		WindowName = LOCTEXT(
			"ActivityRange7Days",
			"7-day window");
	}
	else if (Duration == FTimespan::FromDays(30.0))
	{
		WindowName = LOCTEXT(
			"ActivityRange30Days",
			"30-day window");
	}

	return FText::Format(
		LOCTEXT("ActivityRangeUtcBounds", "{0} | UTC {1} to {2}"),
		WindowName,
		FText::FromString(TimeRange.StartUtc.ToIso8601()),
		FText::FromString(TimeRange.EndUtc.ToIso8601()));
}

FProjectActivityTimeRange
FProjectActivityVisualizerModule::MakeSelectedTimeRange() const
{
	FProjectActivityTimeRange TimeRange;

	FTimespan Lookback(0);

	switch (SelectedTimePreset)
	{
	case EProjectActivityTimePreset::Last24Hours:
		Lookback = FTimespan::FromHours(24.0);
		break;

	case EProjectActivityTimePreset::Last7Days:
		Lookback = FTimespan::FromDays(7.0);
		break;

	case EProjectActivityTimePreset::Last30Days:
		Lookback = FTimespan::FromDays(30.0);
		break;

	case EProjectActivityTimePreset::AllTime:
	default:
		return TimeRange;
	}

	TimeRange.bEnabled = true;
	TimeRange.EndUtc = FDateTime::UtcNow();
	TimeRange.StartUtc = TimeRange.EndUtc - Lookback;

	return TimeRange;
}

void FProjectActivityVisualizerModule::StartupModule()
{
	const bool bStorageReady = ActivityStorage.Initialize();

	if (!bStorageReady)
	{
		UE_LOG(LogProjectActivity,Error,TEXT("Activity storage initialization failed."));
	}

	else
	{
		int64 RecordCount = 0;

		if (ActivityStorage.GetRecordCount(RecordCount))
		{
			UE_LOG(LogProjectActivity , Log , TEXT("Stored activity records: %lld") , RecordCount);
		}
		else
		{
			UE_LOG(LogProjectActivity , Error , TEXT("Failed to read stored activity count."));
		}
	}

	UE_LOG(LogProjectActivity, Log, TEXT("Project Activity Visualizer started."));

	PackageSavedDelegateHandle =UPackage::PackageSavedWithContextEvent.AddRaw(this,&FProjectActivityVisualizerModule::OnPackageSaved);

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FProjectActivityVisualizerStyle::Initialize();
	FProjectActivityVisualizerStyle::ReloadTextures();

	FProjectActivityVisualizerCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FProjectActivityVisualizerCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FProjectActivityVisualizerModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FProjectActivityVisualizerModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ProjectActivityVisualizerTabName, FOnSpawnTab::CreateRaw(this, &FProjectActivityVisualizerModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FProjectActivityVisualizerTabTitle", "ProjectActivityVisualizer"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	if (bStorageReady && !IsRunningCommandlet())
	{
		AutoImportTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FProjectActivityVisualizerModule::TickAutoImport), 5.0f);
	}
}

void FProjectActivityVisualizerModule::ShutdownModule()
{
	SourceControlQuery.Reset();

	if (AutoImportTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(AutoImportTickerHandle);

		AutoImportTickerHandle.Reset();
	}

	ResetAutoImportState();

	if (PackageSavedDelegateHandle.IsValid())
	{
		UPackage::PackageSavedWithContextEvent.Remove(PackageSavedDelegateHandle);

		PackageSavedDelegateHandle.Reset();
	}

	if (TSharedPtr<SWindow> OpenWindow = ReferenceDetailsWindow.Pin())
	{
		OpenWindow->RequestDestroyWindow();
	}

	ReferenceDetailsWindow.Reset();

	if (TSharedPtr<SWindow> OpenWindow = AliasDetailsWindow.Pin())
	{
		OpenWindow->RequestDestroyWindow();
	}

	AliasDetailsWindow.Reset();

	if (TSharedPtr<SDockTab> OpenTab = DashboardTab.Pin())
	{
		OpenTab->RequestCloseTab();
	}

	DashboardTab.Reset();
	DashboardPages.Reset();

	PackageStatsList.Reset();
	PackageStatsStatusText.Reset();
	RecordCountText.Reset();
	TimeRangeText.Reset();

	CategoryStatsList.Reset();
	CategoryStatsStatusText.Reset();

	DailyStatsList.Reset();
	DailyStatsStatusText.Reset();

	MultiContributorList.Reset();
	MultiContributorStatusText.Reset();

	HotspotStatusText.Reset();
	HotspotDetailsText.Reset();
	ActivityStorage.Close();

	UE_LOG(LogProjectActivity, Log, TEXT("Project Activity Visualizer stopped."));

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FProjectActivityVisualizerStyle::Shutdown();

	FProjectActivityVisualizerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectActivityVisualizerTabName);
}

TSharedRef<SDockTab>
FProjectActivityVisualizerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	SourceControlQuery =
		MakeShared<FProjectActivitySourceControlQuery>();

	SourceControlQuery->Storage = &ActivityStorage;

	SourceControlQuery->TryShowCachedSnapshot(
		FString(),
		FString());

	const TWeakPtr<FProjectActivitySourceControlQuery>
		WeakSourceControlQuery = SourceControlQuery;

	const auto MakeActionButton =
		[](const FText& Label,
			const FOnClicked& OnClicked) -> TSharedRef<SWidget>
		{
			return SNew(SButton)
				.Text(Label)
				.OnClicked(OnClicked);
		};

	const auto MakeTimePresetButton =
		[this](const FText& Label,
			EProjectActivityTimePreset Preset) -> TSharedRef<SWidget>
		{
			return SNew(SButton)
				.Text(Label)
				.IsEnabled_Lambda(
					[this, Preset]()
					{
						return SelectedTimePreset != Preset;
					})
				.OnClicked(
					FOnClicked::CreateRaw(
						this,
						&FProjectActivityVisualizerModule::OnTimeRangeClicked,
						Preset));
		};

	const auto MakePageButton =
		[this](const FText& Label,
			int32 PageIndex) -> TSharedRef<SWidget>
		{
			return SNew(SButton)
				.Text(Label)
				.IsEnabled_Lambda(
					[this, PageIndex]()
					{
						return !DashboardPages.IsValid()
							|| DashboardPages->GetActiveWidgetIndex()
							!= PageIndex;
					})
				.OnClicked(
					FOnClicked::CreateRaw(
						this,
						&FProjectActivityVisualizerModule::OnDashboardPageClicked,
						PageIndex));
		};

	// 첫 번째 페이지: 활동 통계
	TSharedRef<SWidget> ActivityPage =
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT(
					"ActivityPageDailyTitle",
					"Daily activity (5-minute buckets, last 7 UTC dates)"))
				.ToolTipText(LOCTEXT(
					"ActivityPageDailyTooltip",
					"Always shows today and the previous 6 UTC dates, "
					"independent of the time range buttons. Today is partial."))
				.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SAssignNew(DailyStatsStatusText, STextBlock)
				.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(DailyStatsList, SScrollBox)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 16.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT(
					"ActivityPageCategoryTitle",
					"Activity by category (5-minute buckets)"))
				.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SAssignNew(CategoryStatsStatusText, STextBlock)
				.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(CategoryStatsList, SScrollBox)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 16.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT(
					"ActivityPagePackagesTitle",
					"Most active packages (5-minute buckets, top 10)"))
				.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SAssignNew(PackageStatsStatusText, STextBlock)
				.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.FillHeight(2.0f)
		[
			SAssignNew(PackageStatsList, SScrollBox)
		];

	// 두 번째 페이지: 별칭과 Hotspot
	TSharedRef<SWidget> CollaborationPage =
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT(
					"CollaborationPageAliasesTitle",
					"Packages with multiple contributor aliases"))
				.ToolTipText(LOCTEXT(
					"CollaborationPageAliasesTooltip",
					"Distinct recorded aliases do not confirm unique people "
					"or simultaneous editing."))
				.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SAssignNew(MultiContributorStatusText, STextBlock)
				.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(MultiContributorList, SScrollBox)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 16.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT(
					"CollaborationPageHotspotTitle",
					"Hotspot review candidates (activity top 10)"))
				.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SAssignNew(HotspotStatusText, STextBlock)
				.AutoWrapText(true)
		]

		+ SVerticalBox::Slot()
		.FillHeight(2.0f)
		[
			SNew(SScrollBox)

				+ SScrollBox::Slot()
				[
					SAssignNew(HotspotDetailsText, STextBlock)
						.AutoWrapText(true)
				]
		];

	// 세 번째 페이지: 커밋 이력
	TSharedRef<SWidget> HistoryPage =
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT(
					"HistoryPageTitle",
					"Selected asset commit history (latest 5)"))
				.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 12.0f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT(
					"HistoryPageHint",
					"Select Show History beside a package on the Activity page. "
					"History is independent of the activity time range. Dates are UTC."))
				.AutoWrapText(true)
		]

	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
				.Padding(8.0f)
				[
					SNew(SScrollBox)

						+ SScrollBox::Slot()
						[
							SNew(STextBlock)
								.Text_Lambda(
									[WeakSourceControlQuery]() -> FText
									{
										const TSharedPtr<FProjectActivitySourceControlQuery>
											PinnedQuery =
											WeakSourceControlQuery.Pin();

										if (PinnedQuery.IsValid())
										{
											return PinnedQuery->DisplayText;
										}

										return LOCTEXT(
											"TabbedHistoryUnavailable",
											"Source control history is unavailable.");
									})
								.AutoWrapText(true)
						]
				]
		];

	TSharedRef<SDockTab> NewTab =
		SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBorder)
				.Padding(16.0f)
				[
					SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT(
									"TabbedDashboardTitle",
									"Project Activity"))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SAssignNew(RecordCountText, STextBlock)
								.AutoWrapText(true)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SWrapBox)
								.UseAllottedSize(true)
								.InnerSlotPadding(FVector2D(8.0f, 6.0f))

								+ SWrapBox::Slot()
								[
									MakeTimePresetButton(
										LOCTEXT("TimePresetAllTime", "All time"),
										EProjectActivityTimePreset::AllTime)
								]

								+ SWrapBox::Slot()
								[
									MakeTimePresetButton(
										LOCTEXT("TimePreset24Hours", "Last 24 hours"),
										EProjectActivityTimePreset::Last24Hours)
								]

								+ SWrapBox::Slot()
								[
									MakeTimePresetButton(
										LOCTEXT("TimePreset7Days", "Last 7 days"),
										EProjectActivityTimePreset::Last7Days)
								]

								+ SWrapBox::Slot()
								[
									MakeTimePresetButton(
										LOCTEXT("TimePreset30Days", "Last 30 days"),
										EProjectActivityTimePreset::Last30Days)
								]

							+ SWrapBox::Slot()
								[
									MakeActionButton(
										LOCTEXT("TabbedRefresh", "Refresh"),
										FOnClicked::CreateRaw(
											this,
											&FProjectActivityVisualizerModule::OnRefreshClicked))
								]

							+ SWrapBox::Slot()
								[
									MakeActionButton(
										LOCTEXT("TabbedExportJson", "Export all JSON"),
										FOnClicked::CreateRaw(
											this,
											&FProjectActivityVisualizerModule::OnExportJsonClicked))
								]

							+ SWrapBox::Slot()
								[
									MakeActionButton(
										LOCTEXT("TabbedExportTeam", "Export to team folder"),
										FOnClicked::CreateRaw(
											this,
											&FProjectActivityVisualizerModule::OnExportTeamJsonClicked))
								]

							+ SWrapBox::Slot()
								[
									MakeActionButton(
										LOCTEXT("TabbedImportJson", "Import JSON"),
										FOnClicked::CreateRaw(
											this,
											&FProjectActivityVisualizerModule::OnImportJsonClicked))
								]
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 12.0f)
						[
							SAssignNew(TimeRangeText, STextBlock)
								.AutoWrapText(true)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 12.0f)
						[
							SNew(SWrapBox)
								.UseAllottedSize(true)
								.InnerSlotPadding(FVector2D(8.0f, 6.0f))

								+ SWrapBox::Slot()
								[
									MakePageButton(
										LOCTEXT("ActivityPageButton", "Activity"),
										0)
								]

								+ SWrapBox::Slot()
								[
									MakePageButton(
										LOCTEXT(
											"CollaborationPageButton",
											"Collaboration / Hotspot"),
										1)
								]

							+ SWrapBox::Slot()
								[
									MakePageButton(
										LOCTEXT(
											"HistoryPageButton",
											"Commit history"),
										2)
								]
						]

					+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SAssignNew(DashboardPages, SWidgetSwitcher)
								.WidgetIndex(0)

								+ SWidgetSwitcher::Slot()
								[
									ActivityPage
								]

								+ SWidgetSwitcher::Slot()
								[
									CollaborationPage
								]

								+ SWidgetSwitcher::Slot()
								[
									HistoryPage
								]
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT(
									"TabbedDashboardRefreshHint",
									"Save assets to record activity. "
									"Use Refresh to update statistics and selected asset history."))
								.AutoWrapText(true)
						]
				]
		];

	DashboardTab = NewTab;

	RefreshDashboard();

	return NewTab;
}

FReply FProjectActivityVisualizerModule::OnDashboardPageClicked(int32 PageIndex)
{
	if (DashboardPages.IsValid() && DashboardPages->GetNumWidgets() > 0)
	{
		const int32 SafeIndex = FMath::Clamp(PageIndex, 0, DashboardPages->GetNumWidgets() - 1);

		DashboardPages->SetActiveWidgetIndex(SafeIndex);
	}

	return FReply::Handled();
}

void FProjectActivityVisualizerModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ProjectActivityVisualizerTabName);
}

void FProjectActivityVisualizerModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FProjectActivityVisualizerCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FProjectActivityVisualizerCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FProjectActivityVisualizerModule::OnPackageSaved(const FString& PackageFileName , UPackage* Package , FObjectPostSaveContext SaveContext)
{
	// 패키지가 없으면 처리하지 않는다.
	if (Package == nullptr)
	{
		return;
	}

	// 수집이 꺼져 있으면 새 기록을 만들지 않는다.
	const UProjectActivitySettings* Settings = GetDefault<UProjectActivitySettings>();

	if (!Settings->bEnableActivityCollection)
	{
		return;
	}

	// 쿠킹, 자동 처리, 자동 저장은 제외한다.
	if (SaveContext.IsCooking() || SaveContext.IsProceduralSave() || SaveContext.IsFromAutoSave())
	{
		return;
	}

	const FString PackagePath = Package->GetName();

	// 설정된 추적 및 제외 경로를 적용한다.
	if (!Settings->ShouldTrackPackage(PackagePath))
	{
		return;
	}

	// 활동 기록을 만들고 패키지 경로를 채운다.
	const UProjectActivityUserSettings* UserSettings =
		GetDefault<UProjectActivityUserSettings>();

	FProjectActivityRecord Record;
	Record.PackagePath = PackagePath;
	Record.ContributorAlias = UserSettings->ContributorAlias.TrimStartAndEnd();

	if (!ActivityStorage.InsertRecord(Record))
	{
		UE_LOG(LogProjectActivity , Error , TEXT("Activity could not be saved: %s") , *Record.PackagePath);

		return;
	}

	// 위에서 가져온 Settings를 재사용한다.
	const FName Category = Settings->ResolveCategory(Record.PackagePath);

	UE_LOG(
		LogProjectActivity,
		Log,
		TEXT("Package category | Package=%s | Category=%s"),
		*Record.PackagePath,
		*Category.ToString());

	const FString EventIdText = Record.EventId.ToString(EGuidFormats::DigitsWithHyphens);

	const FString TimestampText = Record.TimestampUtc.ToIso8601();

	UE_LOG(
		LogProjectActivity,
		Log,
		TEXT("Activity saved to SQLite | Id=%s | Package=%s | Contributor=%s | Utc=%s | Type=%d"),
		*EventIdText,
		*Record.PackagePath,
		*Record.ContributorAlias,
		*TimestampText,
		static_cast<int32>(Record.ActivityType));
}

void FProjectActivityVisualizerModule::RefreshDashboard()
{
	if (!RecordCountText.IsValid()
		|| !PackageStatsList.IsValid()
		|| !PackageStatsStatusText.IsValid()
		|| !TimeRangeText.IsValid()
		|| !CategoryStatsList.IsValid()
		|| !CategoryStatsStatusText.IsValid()
		|| !DailyStatsList.IsValid()
		|| !DailyStatsStatusText.IsValid()
		|| !MultiContributorList.IsValid()
		|| !MultiContributorStatusText.IsValid()
		|| !HotspotStatusText.IsValid()
		|| !HotspotDetailsText.IsValid())
	{
		return;
	}

	// 이전에 표시한 행을 제거한다.
	PackageStatsList->ClearChildren();
	HotspotDetailsText->SetText(FText::GetEmpty());
	HotspotStatusText->SetText(LOCTEXT("HotspotAwaitingActivityData", "Hotspot analysis is unavailable until activity data is loaded."));
	const FProjectActivityTimeRange TimeRange = MakeSelectedTimeRange();

	TimeRangeText->SetText(FText::Format(LOCTEXT("SelectedActivityTimeRange", "Time range: {0}"), MakeActivityTimeRangeText(TimeRange)));

	RefreshCategoryStats(TimeRange);
	RefreshDailyStats();
	RefreshMultiContributorStats(TimeRange); 


	int64 RecordCount = 0;

	if (!ActivityStorage.GetRecordCount(RecordCount , TimeRange))
	{
		RecordCountText->SetText(
			LOCTEXT(
				"ActivityCountError",
				"Could not read activity records. See Output Log."));

		PackageStatsStatusText->SetText(
			LOCTEXT(
				"DashboardRefreshError",
				"Could not refresh dashboard."));

		UE_LOG(
			LogProjectActivity,
			Error,
			TEXT("Dashboard failed to read activity count."));

		return;
	}

	RecordCountText->SetText(
		FText::Format(
			LOCTEXT(
				"ActivityCountFormat",
				"Stored activity records: {0}"),
			FText::AsNumber(RecordCount)));

	TArray<FProjectActivityBucketStats> TopPackages;

	if (!ActivityStorage.GetPackageActivityBucketStats(10, TopPackages, TimeRange))
	{
		PackageStatsStatusText->SetText(
			LOCTEXT(
				"PackageStatsError",
				"Could not read package stats. See Output Log."));

		UE_LOG(LogProjectActivity , Error , TEXT("Dashboard failed to read package stats."));

		return;
	}

	TArray<FProjectActivityHotspotEvidence> HotspotEvidence;

	BuildHotspotEvidence(TopPackages, TimeRange, HotspotEvidence);

	RefreshHotspotCandidates(HotspotEvidence, TimeRange);

	if (TopPackages.IsEmpty())
	{
		PackageStatsStatusText->SetText(
			LOCTEXT(
				"NoPackageStats",
				"No save events recorded yet."));

		return;
	}

	const int32 HotAssetThreshold =
		FMath::Max(
			1,
			GetDefault<UProjectActivitySettings>()
			->HotAssetMinSaveCount);

	PackageStatsStatusText->SetText(
		FText::Format(
			LOCTEXT(
				"ActivityBucketPackageSummary",
				"Showing {0} packages, ranked by 5-minute activity buckets. "
				"High save activity: {1}+ raw saves in the selected time range."),
			FText::AsNumber(TopPackages.Num()),
			FText::AsNumber(HotAssetThreshold)));

	for (const FProjectActivityBucketStats& Stats : TopPackages)
	{
		const bool bHighSaveActivity = Stats.RawSaveCount >= HotAssetThreshold;

		UE_LOG(LogProjectActivity, Log, TEXT("Activity buckets | Package=%s | RawSaves=%lld | Buckets5Min=%lld"), *Stats.PackagePath, Stats.RawSaveCount, Stats.ActivityBucketCount);

		
		PackageStatsList->AddSlot().Padding(0.0f, 3.0f)
			[
				SNew(SBorder)
					.Padding(FMargin(8.0f, 6.0f))
					[
						SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Top)
							[
								SNew(STextBlock)
									.Text(FText::FromString(Stats.PackagePath))
									.ToolTipText(FText::FromString(Stats.PackagePath))
									.AutoWrapText(true)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Top)
							.Padding(12.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SVerticalBox)

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
											.Text(
												FText::Format(
													LOCTEXT(
														"PackageActivityBucketCount",
														"{0} activity buckets (5 min)"),
													FText::AsNumber(
														Stats.ActivityBucketCount)))
											.ToolTipText(LOCTEXT(
												"PackageActivityBucketMeaning",
												"Repeated saves of the same package under the same alias "
												"within one fixed 5-minute UTC interval count as one activity bucket."))
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 3.0f, 0.0f, 0.0f)
									[
										SNew(STextBlock)
											.Text(
												FText::Format(
													LOCTEXT(
														"PackageRawSaveCount",
														"{0} raw saves"),
													FText::AsNumber(
														Stats.RawSaveCount)))
									]
							]


						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Top)
							.Padding(12.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SButton)
									.Text(LOCTEXT("FindAssetButtonLabel", "Find Asset"))
									.ToolTipText(LOCTEXT("FindAssetButtonTooltip" , "Locate this asset in the Content Browser."))
									.OnClicked(FOnClicked::CreateRaw(this , &FProjectActivityVisualizerModule::OnFindAssetClicked , Stats.PackagePath))
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Top)
							.Padding(8.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SButton)
									.Text(LOCTEXT(
										"ShowPackageReferences",
										"Show References"))
									.ToolTipText(LOCTEXT(
										"ShowPackageReferencesTooltip",
										"Write current direct package references under /Game/ "
										"to Output Log. Independent of the activity time range."))
									.OnClicked(
										FOnClicked::CreateRaw(
											this,
											&FProjectActivityVisualizerModule::OnShowReferencesClicked,
											Stats.PackagePath))
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Top)
							.Padding(8.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SButton)
									.Text(LOCTEXT(
										"ShowPackageHistory",
										"Show History"))
									.ToolTipText(LOCTEXT(
										"ShowPackageHistoryTooltip",
										"Show this asset's source control status and recent commit history."))
									.IsEnabled_Lambda(
										[WeakQuery = TWeakPtr<FProjectActivitySourceControlQuery>(
											SourceControlQuery)]() -> bool
										{
											const TSharedPtr<FProjectActivitySourceControlQuery>
												PinnedQuery = WeakQuery.Pin();

											return PinnedQuery.IsValid()
												&& !PinnedQuery->bPending;
										})
									.OnClicked(
										FOnClicked::CreateRaw(
											this,
											&FProjectActivityVisualizerModule::OnShowHistoryClicked,
											Stats.PackagePath))
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Top)
							.Padding(12.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
									.Text(LOCTEXT(
										"HighSaveActivityLabel",
										"High save activity"))
									.ColorAndOpacity(
										FSlateColor(
											FLinearColor(
												1.0f,
												0.75f,
												0.2f,
												1.0f)))
									.Visibility(
										bHighSaveActivity
										? EVisibility::Visible
										: EVisibility::Collapsed)
									.ToolTipText(
										FText::Format(
											LOCTEXT(
												"HighSaveActivityTooltip",
												"At least {0} save events in the selected time range."),
											FText::AsNumber(HotAssetThreshold)))
							]

					]
			];
	}

}

void FProjectActivityVisualizerModule::RefreshCategoryStats(
	const FProjectActivityTimeRange& TimeRange)
{
	if (!CategoryStatsList.IsValid() || !CategoryStatsStatusText.IsValid())
	{
		return;
	}

	CategoryStatsList->ClearChildren();

	TArray<FProjectActivityBucketStats> AllPackageStats;

	// 0을 전달해서 모든 패키지를 조회한다.
	if (!ActivityStorage.GetPackageActivityBucketStats(0, AllPackageStats, TimeRange))
	{
		CategoryStatsStatusText->SetText(
			LOCTEXT(
				"CategoryStatsQueryError",
				"Could not read category stats. See Output Log."));

		UE_LOG(
			LogProjectActivity,
			Error,
			TEXT("Dashboard failed to read category source data."));

		return;
	}

	const UProjectActivitySettings* Settings = GetDefault<UProjectActivitySettings>();

	const TArray<FProjectActivityCategoryStats> CategoryStats = FProjectActivityAnalyzer::SummarizeCategories(AllPackageStats, *Settings);

	int64 TotalSaves = 0;
	int64 TotalBuckets = 0;

	for (const FProjectActivityCategoryStats& Stats : CategoryStats)
	{
		TotalSaves += Stats.SaveCount;
		TotalBuckets += Stats.ActivityBucketCount;
	}

	if (TotalBuckets <= 0)
	{
		CategoryStatsStatusText->SetText(
			LOCTEXT(
				"NoCategoryBucketStats",
				"No activity buckets in the selected time range."));

		return;
	}

	CategoryStatsStatusText->SetText(
		FText::Format(
			LOCTEXT(
				"CategoryBucketSummary",
				"{0} categories / {1} activity buckets / {2} raw saves. "
				"Bars show each category's share of activity buckets."),
			FText::AsNumber(CategoryStats.Num()),
			FText::AsNumber(TotalBuckets),
			FText::AsNumber(TotalSaves)));

	FNumberFormattingOptions PercentOptions;
	PercentOptions.MinimumFractionalDigits = 1;
	PercentOptions.MaximumFractionalDigits = 1;

	for (const FProjectActivityCategoryStats& Stats : CategoryStats)
	{
		const double Share = static_cast<double>(Stats.ActivityBucketCount) / static_cast<double>(TotalBuckets);

		CategoryStatsList->AddSlot()
			.Padding(0.0f, 3.0f)
			[
				SNew(SBorder)
					.Padding(FMargin(8.0f, 6.0f))
					[
						SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
											.Text(FText::FromName(
												Stats.CategoryName))
											.AutoWrapText(true)
									]

									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(12.0f, 0.0f, 0.0f, 0.0f)
									[
										SNew(STextBlock)
											.Text(
												FText::Format(
													LOCTEXT(
														"CategoryBucketCounts",
														"{0} buckets / {1} raw saves ({2})"),
													FText::AsNumber(
														Stats.ActivityBucketCount),
													FText::AsNumber(
														Stats.SaveCount),
													FText::AsPercent(
														Share,
														&PercentOptions)))
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(SBox)
									.HeightOverride(12.0f)
									[
										SNew(SProgressBar)
											.Percent(
												TOptional<float>(
													static_cast<float>(
														Share)))
									]
							]
					]
			];

		UE_LOG(
			LogProjectActivity,
			Log,
			TEXT("Category buckets | Category=%s | RawSaves=%lld | Buckets5Min=%lld"),
			*Stats.CategoryName.ToString(),
			Stats.SaveCount,
			Stats.ActivityBucketCount);
	}
}

void FProjectActivityVisualizerModule::RefreshDailyStats()
{
	if (!DailyStatsList.IsValid() || !DailyStatsStatusText.IsValid())
	{
		return;
	}

	DailyStatsList->ClearChildren();

	const FDateTime NowUtc = FDateTime::UtcNow();

	TArray<FProjectActivityDailyStats> DailyStats;

	if (!ActivityStorage.GetRecentDailySaveCounts(7, DailyStats, NowUtc))
	{
		DailyStatsStatusText->SetText(
			LOCTEXT(
				"DailyStatsQueryError",
				"Could not read daily stats. See Output Log."));

		UE_LOG(LogProjectActivity, Error, TEXT("Failed to read recent daily stats."));

		return;
	}

	if (DailyStats.IsEmpty())
	{
		DailyStatsStatusText->SetText(
			LOCTEXT(
				"DailyStatsEmpty",
				"No daily stats available."));

		return;
	}

	int64 TotalSaves = 0;
	int64 TotalBuckets = 0;
	int64 MaxBuckets = 0;

	for (const FProjectActivityDailyStats& Stats : DailyStats)
	{
		TotalSaves += Stats.SaveCount;
		TotalBuckets += Stats.ActivityBucketCount;

		MaxBuckets = FMath::Max(MaxBuckets, Stats.ActivityBucketCount);
	}

	if (TotalBuckets == 0)
	{
		DailyStatsStatusText->SetText(
			LOCTEXT(
				"DailyBucketsNoActivity",
				"No activity in these 7 UTC dates. Today is partial."));
	}
	else
	{
		DailyStatsStatusText->SetText(
			FText::Format(
				LOCTEXT(
					"DailyBucketSummary",
					"{0} to {1} (UTC) / {2} activity buckets / "
					"{3} raw saves / Full bar: {4} buckets. Today is partial."),
				FText::FromString(DailyStats[0].DateUtc),
				FText::FromString(DailyStats.Last().DateUtc),
				FText::AsNumber(TotalBuckets),
				FText::AsNumber(TotalSaves),
				FText::AsNumber(MaxBuckets)));
	}

	for (const FProjectActivityDailyStats& Stats : DailyStats)
	{
		const float BarFraction =
			MaxBuckets > 0
			? static_cast<float>(
				static_cast<double>(Stats.ActivityBucketCount)
				/ static_cast<double>(MaxBuckets))
			: 0.0f;

		DailyStatsList->AddSlot()
			.Padding(0.0f, 3.0f)
			[
				SNew(SBorder)
					.Padding(FMargin(8.0f, 6.0f))
					[
						SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SBox)
									.WidthOverride(110.0f)
									[
										SNew(STextBlock)
											.Text(FText::FromString(
												Stats.DateUtc))
									]
							]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							.Padding(8.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SBox)
									.HeightOverride(14.0f)
									[
										SNew(SProgressBar)
											.Percent(
												TOptional<float>(
													BarFraction))
									]
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SBox)
									.WidthOverride(170.0f)
									.HAlign(HAlign_Right)
									[
										SNew(SVerticalBox)

											+ SVerticalBox::Slot()
											.AutoHeight()
											.HAlign(HAlign_Right)
											[
												SNew(STextBlock)
													.Text(
														FText::Format(
															LOCTEXT(
																"DailyActivityBuckets",
																"{0} buckets"),
															FText::AsNumber(
																Stats.ActivityBucketCount)))
											]

										+ SVerticalBox::Slot()
											.AutoHeight()
											.HAlign(HAlign_Right)
											.Padding(0.0f, 3.0f, 0.0f, 0.0f)
											[
												SNew(STextBlock)
													.Text(
														FText::Format(
															LOCTEXT(
																"DailyRawSaves",
																"{0} raw saves"),
															FText::AsNumber(
																Stats.SaveCount)))
											]
									]
							]
					]
			];

		UE_LOG(
			LogProjectActivity,
			Log,
			TEXT("Daily buckets | DateUtc=%s | RawSaves=%lld | Buckets5Min=%lld"),
			*Stats.DateUtc,
			Stats.SaveCount,
			Stats.ActivityBucketCount);
	}
}

void FProjectActivityVisualizerModule::ResetAutoImportState()
{
	AutoImportFolder.Empty();
	AutoImportFiles.Reset();
	bAutoImportFolderWarningShown = false;
}

bool FProjectActivityVisualizerModule::TickAutoImport(
	float DeltaTime)
{
	const UProjectActivityUserSettings* Settings = GetDefault<UProjectActivityUserSettings>();

	if (!Settings->bEnableAutomaticImport)
	{
		ResetAutoImportState();
		return true;
	}

	FString Folder = Settings->TeamExchangeFolder.TrimStartAndEnd();

	FPaths::NormalizeDirectoryName(Folder);

	if (Folder != AutoImportFolder)
	{
		ResetAutoImportState();
		AutoImportFolder = Folder;
	}

	if (Folder.IsEmpty()
		|| FPaths::IsRelative(Folder)
		|| !IFileManager::Get().DirectoryExists(*Folder))
	{
		if (!bAutoImportFolderWarningShown)
		{
			UE_LOG(LogProjectActivity, Warning, TEXT("Automatic import needs an existing absolute folder path: %s"), *Folder);

			bAutoImportFolderWarningShown = true;
		}

		return true;
	}

	bAutoImportFolderWarningShown = false;

	TArray<FString> FileNames;

	IFileManager::Get().FindFiles(FileNames, *FPaths::Combine(Folder, TEXT("*.json")), true, false);

	FileNames.Sort();

	for (const FString& FileName : FileNames)
	{
		const FString FilePath = FPaths::Combine(Folder, FileName);

		const int64 FileSize = IFileManager::Get().FileSize(*FilePath);

		if (FileSize <= 0)
		{
			continue;
		}

		const FDateTime ModifiedTime =
			IFileManager::Get().GetTimeStamp(*FilePath);

		FAutoImportFileState& State =
			AutoImportFiles.FindOrAdd(FilePath);

		// 처음 발견했거나 변경된 파일이면 다음 검사까지 기다린다.
		if (State.Size != FileSize
			|| State.ModifiedTime != ModifiedTime)
		{
			State.Size = FileSize;
			State.ModifiedTime = ModifiedTime;
			State.bImported = false;
			State.NextRetryTime = 0.0;

			continue;
		}

		if (State.bImported
			|| FPlatformTime::Seconds() < State.NextRetryTime)
		{
			continue;
		}

		int64 ImportedCount = 0;
		int64 SkippedCount = 0;

		if (!ActivityStorage.ImportRecordsFromJson(
			FilePath,
			ImportedCount,
			SkippedCount))
		{
			State.NextRetryTime =
				FPlatformTime::Seconds() + 30.0;

			UE_LOG(LogProjectActivity , Warning , TEXT("Auto import failed; retry in 30 seconds: %s") , *FilePath);

			break;
		}

		State.bImported = true;

		UE_LOG(
			LogProjectActivity,
			Log,
			TEXT("Auto import | New=%lld | Skipped=%lld | File=%s"),
			ImportedCount,
			SkippedCount,
			*FilePath);

		if (ImportedCount > 0 && DashboardTab.IsValid())
		{
			RefreshDashboard();

			if (PackageStatsStatusText.IsValid())
			{
				PackageStatsStatusText->SetText(
					FText::Format(
						LOCTEXT(
							"AutomaticImportSucceeded",
							"Automatically imported {0} new records."),
						FText::AsNumber(ImportedCount)));
			}
		}

		break;
	}

	return true;
}

void FProjectActivityVisualizerModule::RefreshMultiContributorStats(const FProjectActivityTimeRange& TimeRange)
{
	if (!MultiContributorList.IsValid()
		|| !MultiContributorStatusText.IsValid())
	{
		return;
	}

	// 새 조회 전에 기존 행을 제거한다.
	MultiContributorList->ClearChildren();

	TArray<FProjectActivityMultiContributorStats> Results;

	if (!ActivityStorage.GetMultiContributorPackages(10, Results, TimeRange))
	{
		MultiContributorStatusText->SetText(
			LOCTEXT(
				"MultiContributorQueryError",
				"Could not read multi-contributor stats. See Output Log."));

		UE_LOG(LogProjectActivity, Error, TEXT("Failed to read multi-contributor stats."));

		return;
	}

	UE_LOG(LogProjectActivity, Log, TEXT("Multi-contributor summary | Range=%s | Showing=%d"), TimeRange.bEnabled ? TEXT("Last7Days") : TEXT("AllTime"), Results.Num());

	if (Results.IsEmpty())
	{
		MultiContributorStatusText->SetText(
			LOCTEXT(
				"NoMultiContributorStats",
				"No packages with 2 or more recorded aliases "
				"in the selected time range."));

		return;
	}

	MultiContributorStatusText->SetText(
		FText::Format(
			LOCTEXT(
				"MultiContributorResultCount",
				"Showing {0} packages with 2 or more recorded aliases (top 10)."),
			FText::AsNumber(Results.Num())));

	for (const FProjectActivityMultiContributorStats& Stats : Results)
	{
		MultiContributorList->AddSlot()
			.Padding(0.0f, 3.0f)
			[
				SNew(SBorder)
					.Padding(FMargin(8.0f, 6.0f))
					[
						SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
											.Text(FText::FromString(
												Stats.PackagePath))
											.ToolTipText(FText::FromString(
												Stats.PackagePath))
											.AutoWrapText(true)
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(12.0f, 0.0f, 0.0f, 0.0f)
									[
										SNew(SButton)
											.Text(LOCTEXT("MultiContributorFindAsset", "Find Asset"))
											.OnClicked(
												FOnClicked::CreateRaw(
													this,
													&FProjectActivityVisualizerModule::OnFindAssetClicked,
													Stats.PackagePath))
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(8.0f, 0.0f, 0.0f, 0.0f)
									[
										SNew(SButton)
											.Text(LOCTEXT("ShowPackageAliases", "Show Aliases"))
											.ToolTipText(LOCTEXT(
												"ShowPackageAliasesTooltip",
												"Write save counts per alias to Output Log "
												"using this list's time range."))
											.OnClicked(
												FOnClicked::CreateRaw(
													this,
													&FProjectActivityVisualizerModule::OnShowAliasesClicked,
													Stats.PackagePath,
													TimeRange))
									]

							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 6.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
									.Text(
										FText::Format(
											LOCTEXT(
												"MultiContributorCounts",
												"{0} aliases / {1} saves"),
											FText::AsNumber(
												Stats.ContributorCount),
											FText::AsNumber(
												Stats.SaveCount)))
									.AutoWrapText(true)
							]
					]
			];


		UE_LOG(LogProjectActivity, Log, TEXT("Multi-contributor | Package=%s | Aliases=%lld | Saves=%lld"), *Stats.PackagePath, Stats.ContributorCount, Stats.SaveCount);
	}
}

FReply FProjectActivityVisualizerModule::OnShowAliasesClicked(FString PackagePath, FProjectActivityTimeRange TimeRange)
{
	TArray<FProjectActivityContributorStats> Results;

	if (!ActivityStorage.GetPackageContributorStats(PackagePath, Results, TimeRange))
	{
		if (MultiContributorStatusText.IsValid())
		{
			MultiContributorStatusText->SetText(
				LOCTEXT("ContributorDetailsFailed", "Could not read alias details. See Output Log."));
		}

		return FReply::Handled();
	}

	int64 TotalSaves = 0;
	int32 NamedAliasCount = 0;

	TSharedRef<SScrollBox> DetailsList = SNew(SScrollBox);

	UE_LOG(LogProjectActivity, Log, TEXT("Alias details | Package=%s | Range=%s"), *PackagePath, TimeRange.bEnabled ? TEXT("Last7Days") : TEXT("AllTime"));

	for (const FProjectActivityContributorStats& Stats : Results)
	{
		TotalSaves += Stats.SaveCount;

		if (!Stats.ContributorAlias.IsEmpty())
		{
			++NamedAliasCount;
		}

		const FText DisplayAlias =
			Stats.ContributorAlias.IsEmpty()
			? LOCTEXT(
				"UnspecifiedContributorAlias",
				"(Unspecified)")
			: FText::FromString(Stats.ContributorAlias);

		DetailsList->AddSlot()
			.Padding(0.0f, 3.0f)
			[
				SNew(SBorder)
					.Padding(FMargin(8.0f, 6.0f))
					[
						SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(DisplayAlias)
									.AutoWrapText(true)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(12.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
									.Text(
										FText::Format(
											LOCTEXT(
												"AliasDetailsSaveCount",
												"{0} saves"),
											FText::AsNumber(
												Stats.SaveCount)))
							]
					]
			];

		UE_LOG(LogProjectActivity, Log, TEXT("Alias saves | Alias=%s | Saves=%lld"), *DisplayAlias.ToString(), Stats.SaveCount);
	}

	if (Results.IsEmpty())
	{
		DetailsList->AddSlot()
			[
				SNew(STextBlock)
					.Text(LOCTEXT(
						"AliasDetailsNoRecords",
						"No save records in this time range."))
					.AutoWrapText(true)
			];
	}

	const FText RangeLabel =
		TimeRange.bEnabled
		? FText::Format(
			LOCTEXT(
				"AliasDetailsFilteredRange",
				"Time range (UTC): {0} to {1}"),
			FText::FromString(
				TimeRange.StartUtc.ToIso8601()),
			FText::FromString(
				TimeRange.EndUtc.ToIso8601()))
		: LOCTEXT(
			"AliasDetailsAllTime",
			"Time range: All time");

	TSharedPtr<SWindow> DetailsWindow = AliasDetailsWindow.Pin();

	const bool bCreateNewWindow = !DetailsWindow.IsValid();

	if (bCreateNewWindow)
	{
		DetailsWindow =
			SNew(SWindow)
			.Title(LOCTEXT("AliasDetailsWindowTitle", "Project Activity - Alias Details"))
			.ClientSize(FVector2D(640.0f, 420.0f))
			.SupportsMinimize(false)
			.SupportsMaximize(true);
	}

	DetailsWindow->SetContent(
		SNew(SBorder)
		.Padding(16.0f)
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(PackagePath))
						.AutoWrapText(true)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
						.Text(RangeLabel)
						.AutoWrapText(true)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(STextBlock)
						.Text(
							FText::Format(
								LOCTEXT(
									"AliasDetailsSummary",
									"Recorded aliases: {0} / Total saves: {1}"),
								FText::AsNumber(NamedAliasCount),
								FText::AsNumber(TotalSaves)))
						.AutoWrapText(true)
				]

			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					DetailsList
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT(
							"AliasDetailsSnapshotHint",
							"Snapshot view. Refresh the dashboard, then click "
							"Show Aliases again to update this window."))
						.AutoWrapText(true)
				]
		]);

	if (bCreateNewWindow)
	{
		AliasDetailsWindow = DetailsWindow;

		FSlateApplication::Get().AddWindow(DetailsWindow.ToSharedRef());
	}
	else
	{
		DetailsWindow->BringToFront(true);
	}

	UE_LOG(LogProjectActivity, Log, TEXT("Alias details total | Saves=%lld"), TotalSaves);

	if (MultiContributorStatusText.IsValid())
	{
		MultiContributorStatusText->SetText(
			FText::Format(
				LOCTEXT(
					"ContributorDetailsOpened",
					"Showing alias details for {0}."),
				FText::FromString(PackagePath)));
	}

	return FReply::Handled();
}

void FProjectActivityVisualizerModule::ShowReferenceDetailsWindow(const FString& PackagePath, const TArray<FName>& Dependencies, const TArray<FName>& Referencers)
{
	// 참조 경로 목록과 Find Asset 버튼을 만든다.
	const auto BuildPackageList = [this](const TArray<FName>& Packages) -> TSharedRef<SScrollBox>
		{
			TSharedRef<SScrollBox> List = SNew(SScrollBox);

			if (Packages.IsEmpty())
			{
				List->AddSlot()
					[
						SNew(STextBlock)
							.Text(LOCTEXT(
								"ReferenceDetailsEmptyList",
								"No matching /Game/ packages."))
							.AutoWrapText(true)
					];

				return List;
			}

			for (const FName& Name : Packages)
			{
				const FString TargetPath = Name.ToString();

				List->AddSlot()
					.Padding(0.0f, 3.0f)
					[
						SNew(SBorder)
							.Padding(FMargin(8.0f, 6.0f))
							[
								SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
											.Text(FText::FromString(
												TargetPath))
											.ToolTipText(FText::FromString(
												TargetPath))
											.AutoWrapText(true)
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(12.0f, 0.0f, 0.0f, 0.0f)
									[
										SNew(SButton)
											.Text(LOCTEXT(
												"ReferenceDetailsFindAsset",
												"Find Asset"))
											.OnClicked(
												FOnClicked::CreateRaw(
													this,
													&FProjectActivityVisualizerModule::OnFindAssetClicked,
													TargetPath))
									]
							]
					];
			}

			return List;
		};

	const TSharedRef<SScrollBox> DependencyList = BuildPackageList(Dependencies);

	const TSharedRef<SScrollBox> ReferencerList = BuildPackageList(Referencers);

	TSharedPtr<SWindow> DetailsWindow = ReferenceDetailsWindow.Pin();

	const bool bCreateNewWindow = !DetailsWindow.IsValid();

	if (bCreateNewWindow)
	{
		DetailsWindow =
			SNew(SWindow)
			.Title(LOCTEXT(
				"ReferenceDetailsWindowTitle",
				"Project Activity - Reference Details"))
			.ClientSize(FVector2D(760.0f, 560.0f))
			.SupportsMinimize(false)
			.SupportsMaximize(true);
	}

	DetailsWindow->SetContent(
		SNew(SBorder)
		.Padding(16.0f)
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(PackagePath))
						.AutoWrapText(true)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(STextBlock)
						.Text(
							FText::Format(
								LOCTEXT(
									"ReferenceDetailsSummary",
									"Dependencies: {0} / Referencers: {1} "
									"(direct /Game/ packages)"),
								FText::AsNumber(
									Dependencies.Num()),
								FText::AsNumber(
									Referencers.Num())))
						.AutoWrapText(true)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT(
							"ReferenceDependenciesHeading",
							"Dependencies - packages used by this package"))
						.AutoWrapText(true)
				]

			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					DependencyList
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 16.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT(
							"ReferenceReferencersHeading",
							"Referencers - packages that use this package"))
						.AutoWrapText(true)
				]

			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					ReferencerList
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 12.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT(
							"ReferenceDetailsSnapshotHint",
							"Shows saved package references, independent of "
							"the activity time range. Save changed assets, "
							"then refresh this view."))
						.AutoWrapText(true)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
						.Text(LOCTEXT(
							"RefreshReferenceDetails",
							"Refresh References"))
						.OnClicked(
							FOnClicked::CreateRaw(
								this,
								&FProjectActivityVisualizerModule::OnShowReferencesClicked,
								PackagePath))
				]
		]);

	if (bCreateNewWindow)
	{
		ReferenceDetailsWindow = DetailsWindow;

		FSlateApplication::Get().AddWindow(DetailsWindow.ToSharedRef());
	}
	else
	{
		DetailsWindow->BringToFront(true);
	}
}

FReply FProjectActivityVisualizerModule::OnShowReferencesClicked(FString PackagePath)
{
	const auto SetStatus = [this](const FText& Message)
		{
			if (PackageStatsStatusText.IsValid())
			{
				PackageStatsStatusText->SetText(Message);
			}
		};

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (AssetRegistry.IsLoadingAssets())
	{
		SetStatus(
			LOCTEXT(
				"ReferenceScanPending",
				"Asset discovery is still running. Please try again shortly."));

		return FReply::Handled();
	}

	const FName PackageName(*PackagePath);

	TArray<FName> RawDependencies;
	TArray<FName> RawReferencers;

	const bool bDependenciesFound =AssetRegistry.GetDependencies(PackageName, RawDependencies, UE::AssetRegistry::EDependencyCategory::Package);

	const bool bReferencersFound = AssetRegistry.GetReferencers(PackageName, RawReferencers, UE::AssetRegistry::EDependencyCategory::Package);

	if (!bDependenciesFound || !bReferencersFound)
	{
		SetStatus(
			LOCTEXT(
				"ReferenceQueryUnavailable",
				"Reference data is unavailable. "
				"Save the asset and try again. See Output Log."));

		UE_LOG(LogProjectActivity, Warning, TEXT("Reference data unavailable | Package=%s | Dependencies=%d | Referencers=%d"),
			*PackagePath,
			static_cast<int32>(bDependenciesFound),
			static_cast<int32>(bReferencersFound));

		return FReply::Handled();
	}

	// 프로젝트 Content의 패키지만 남기고 중복과 자기 참조를 제외한다.
	const auto CollectProjectPackages = [PackageName](const TArray<FName>& Packages)
		{
			TSet<FName> UniquePackages;

			for (const FName& Name : Packages)
			{
				if (Name == PackageName)
				{
					continue;
				}

				if (Name.ToString().StartsWith(TEXT("/Game/"), ESearchCase::IgnoreCase))
				{
					UniquePackages.Add(Name);
				}
			}

			TArray<FName> Result = UniquePackages.Array();

			Result.Sort(
				[](const FName& A, const FName& B)
				{
					return A.LexicalLess(B);
				});

			return Result;
		};

	const TArray<FName> Dependencies = CollectProjectPackages(RawDependencies);

	const TArray<FName> Referencers = CollectProjectPackages(RawReferencers);

	UE_LOG(LogProjectActivity, Log, TEXT("References | Package=%s | GameDependencies=%d | GameReferencers=%d"),
		*PackagePath,
		Dependencies.Num(),
		Referencers.Num());

	for (const FName& Dependency : Dependencies)
	{
		UE_LOG(LogProjectActivity, Log, TEXT("Depends on | Package=%s"), *Dependency.ToString());
	}

	for (const FName& Referencer : Referencers)
	{
		UE_LOG(LogProjectActivity, Log, TEXT("Referenced by | Package=%s"), *Referencer.ToString());
	}

	ShowReferenceDetailsWindow(PackagePath, Dependencies, Referencers);

	SetStatus(FText::Format(
			LOCTEXT(
				"ReferenceDetailsOpened",
				"Showing reference details for {0}."),
			FText::FromString(PackagePath)));

	return FReply::Handled();
}

void FProjectActivityVisualizerModule::LogActivityBucketStats(const FProjectActivityTimeRange& TimeRange)
{
	TArray<FProjectActivityBucketStats> Results;

	if (!ActivityStorage.GetPackageActivityBucketStats(10, Results, TimeRange))
	{
		UE_LOG(LogProjectActivity, Error, TEXT("Failed to read activity bucket stats."));

		return;
	}

	UE_LOG(LogProjectActivity, Log, TEXT("Activity bucket summary | Range=%s | BucketMinutes=5 | Showing=%d"), TimeRange.bEnabled ? TEXT("Last7Days") : TEXT("AllTime"), Results.Num());

	for (const FProjectActivityBucketStats& Stats : Results)
	{
		UE_LOG(LogProjectActivity, Log, TEXT("Activity buckets | Package=%s | RawSaves=%lld | Buckets5Min=%lld"), *Stats.PackagePath, Stats.RawSaveCount, Stats.ActivityBucketCount);
	}
}

void FProjectActivityVisualizerModule::LogSourceControlStatus()
{
	ISourceControlModule& SourceControl = ISourceControlModule::Get();

	ISourceControlProvider& Provider = SourceControl.GetProvider();

	const FString ProviderName = Provider.GetName().ToString();

	const bool bEnabled = SourceControl.IsEnabled();

	const bool bAvailable = Provider.IsAvailable();

	UE_LOG(LogProjectActivity, Log, TEXT("Source control | Provider=%s | Enabled=%d | Available=%d"), *ProviderName, static_cast<int32>(bEnabled), static_cast<int32>(bAvailable));
}

void FProjectActivityVisualizerModule::QueryPackageSourceControl(const FString& PackagePath)
{
	if (!SourceControlQuery.IsValid())
	{
		SourceControlQuery = MakeShared<FProjectActivitySourceControlQuery>();
	}

	FProjectActivitySourceControlQuery& Query = *SourceControlQuery;

	Query.Storage = &ActivityStorage;

	if (Query.bPending)
	{
		UE_LOG(LogProjectActivity, Log, TEXT("Asset source control query is already running."));

		return;
	}

	Query.PackagePath = PackagePath;

	if (!PackagePath.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(PackagePath))
	{
		Query.SetFailure(TEXT("Select a valid package under /Game/."));

		return;
	}

	Query.DisplayText = FText::Format(
		LOCTEXT(
			"LoadingSelectedAssetHistory",
			"Loading source control status and history for {0}..."),
		FText::FromString(Query.PackagePath));

	ISourceControlModule& SourceControl = ISourceControlModule::Get();

	if (!SourceControl.IsEnabled())
	{
		Query.SetFailure(
			TEXT("Source control is disabled. Connect a provider, then click Refresh."));

		return;
	}

	ISourceControlProvider& Provider = SourceControl.GetProvider();

	if (!Provider.IsAvailable())
	{
		Query.SetFailure(
			TEXT("Source control provider is unavailable. Check the connection, then click Refresh."));

		return;
	}

	FString FileName;

	if (!FPackageName::DoesPackageExist(Query.PackagePath, &FileName))
	{
		Query.SetFailure(
			TEXT("The selected package file was not found. It may have been moved or deleted."));

		return;
	}

	FileName = FPaths::ConvertRelativePathToFull(FileName);
	FPaths::NormalizeFilename(FileName);

	Query.FileName = FileName;
	Query.ProviderName = Provider.GetName();

	const TMap<ISourceControlProvider::EStatus, FString>
		ProviderStatus = Provider.GetStatus();

	Query.RepositoryRoot = ProviderStatus.FindRef(ISourceControlProvider::EStatus::Repository);

	Query.BranchName = ProviderStatus.FindRef(ISourceControlProvider::EStatus::Branch);

	TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> Operation = ISourceControlOperation::Create<FUpdateStatus>();

	Operation->SetUpdateModifiedState(true);
	Operation->SetUpdateHistory(true);

	TArray<FString> Files;
	Files.Add(FileName);

	Query.bPending = true;

	const ECommandResult::Type StartResult =
		Provider.Execute(
			Operation,
			Files,
			EConcurrency::Asynchronous,
			FSourceControlOperationComplete::CreateSP(
				SourceControlQuery.ToSharedRef(),
				&FProjectActivitySourceControlQuery::OnCompleted));

	if (StartResult != ECommandResult::Succeeded)
	{
		Query.bPending = false;

		Query.SetFailure(
			TEXT("Failed to start the asset source control query."));
	}
}

FReply FProjectActivityVisualizerModule::OnShowHistoryClicked(FString PackagePath)
{
	QueryPackageSourceControl(PackagePath);

	if (DashboardPages.IsValid())
	{
		DashboardPages->SetActiveWidgetIndex(2);
	}

	return FReply::Handled();
}

FReply FProjectActivityVisualizerModule::OnRefreshClicked()
{
	LogSourceControlStatus();

	if (SourceControlQuery.IsValid() && !SourceControlQuery->PackagePath.IsEmpty())
	{
		QueryPackageSourceControl(SourceControlQuery->PackagePath);
	}

	RefreshDashboard();

	return FReply::Handled();
}

FReply FProjectActivityVisualizerModule::OnExportJsonClicked()
{
	const FString ExportDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ProjectActivity"), TEXT("Exports")));

	const FString FileName = FString::Printf(TEXT("activity_%s_%s.json"), *FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")), *FGuid::NewGuid().ToString(EGuidFormats::Digits));

	const FString FilePath = FPaths::Combine(ExportDirectory, FileName);

	int64 ExportedCount = 0;

	if (!ActivityStorage.ExportAllRecordsToJson(FilePath, ExportedCount))
	{
		if (PackageStatsStatusText.IsValid())
		{
			PackageStatsStatusText->SetText(
				LOCTEXT(
					"ActivityExportFailed",
					"JSON export failed. See Output Log."));
		}

		return FReply::Handled();
	}

	if (PackageStatsStatusText.IsValid())
	{
		PackageStatsStatusText->SetText(
			FText::Format(
				LOCTEXT(
					"ActivityExportSucceeded",
					"Exported {0} records to: {1}"),
				FText::AsNumber(ExportedCount),
				FText::FromString(FilePath)));
	}

	UE_LOG(LogProjectActivity, Log, TEXT("Exported %lld activity records to: %s"), ExportedCount, *FilePath);

	return FReply::Handled();
}

FReply FProjectActivityVisualizerModule::OnExportTeamJsonClicked()
{
	const auto SetStatus = [this](const FText& Message)
		{
			if (PackageStatsStatusText.IsValid())
			{
				PackageStatsStatusText->SetText(Message);
			}
		};

	const UProjectActivityUserSettings* UserSettings = GetDefault<UProjectActivityUserSettings>();

	FString Folder = UserSettings->TeamExchangeFolder.TrimStartAndEnd();

	FPaths::NormalizeDirectoryName(Folder);

	if (Folder.IsEmpty() || FPaths::IsRelative(Folder) || !IFileManager::Get().DirectoryExists(*Folder))
	{
		SetStatus(
			LOCTEXT(
				"InvalidTeamExchangeFolder",
				"Set an existing absolute Team Exchange Folder in Editor Preferences."));

		UE_LOG(LogProjectActivity, Error, TEXT("Invalid team exchange folder: %s"), *Folder);

		return FReply::Handled();
	}

	const FString FileName = FString::Printf(TEXT("activity_%s_%s.json"), *FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")), *FGuid::NewGuid().ToString(EGuidFormats::Digits));

	const FString FinalFilePath = FPaths::Combine(Folder, FileName);

	const FString TempFilePath =
		FinalFilePath + TEXT(".tmp");

	// 실패했을 때 남은 임시 파일을 정리한다.
	ON_SCOPE_EXIT
	{
		if (IFileManager::Get().FileExists(*TempFilePath))
		{
			IFileManager::Get().Delete(*TempFilePath);
		}
	};

	int64 ExportedCount = 0;

	if (!ActivityStorage.ExportAllRecordsToJson(TempFilePath, ExportedCount))
	{
		SetStatus(
			LOCTEXT(
				"TeamExportWriteFailed",
				"Team JSON export failed. See Output Log."));

		return FReply::Handled();
	}

	// 완성된 파일만 .json 이름으로 공개한다.
	// 같은 이름의 파일이 있다면 덮어쓰지 않는다.
	if (!IFileManager::Get().Move(*FinalFilePath, *TempFilePath, false))
	{
		SetStatus(
			LOCTEXT(
				"TeamExportMoveFailed",
				"Could not finalize team JSON export. See Output Log."));

		UE_LOG(
			LogProjectActivity,
			Error,
			TEXT("Failed to finalize team export: %s"),
			*FinalFilePath);

		return FReply::Handled();
	}

	SetStatus(
		FText::Format(
			LOCTEXT(
				"TeamExportSucceeded",
				"Exported {0} records to team folder: {1}"),
			FText::AsNumber(ExportedCount),
			FText::FromString(FinalFilePath)));

	UE_LOG(LogProjectActivity, Log, TEXT("Team export | Records=%lld | File=%s"), ExportedCount, *FinalFilePath);

	return FReply::Handled();
}

FReply FProjectActivityVisualizerModule::OnImportJsonClicked()
{
	IDesktopPlatform* DesktopPlatform =
		FDesktopPlatformModule::Get();

	if (DesktopPlatform == nullptr)
	{
		UE_LOG(LogProjectActivity, Error, TEXT("The file dialog is unavailable."));

		return FReply::Handled();
	}

	TArray<FString> SelectedFiles;

	const bool bSelected =
		DesktopPlatform->OpenFileDialog(
			nullptr,
			TEXT("Import Project Activity JSON"),
			FPaths::ConvertRelativePathToFull(
				FPaths::ProjectSavedDir()),
			TEXT(""),
			TEXT("Activity JSON (*.json)|*.json"),
			EFileDialogFlags::Multiple,
			SelectedFiles);

	if (!bSelected || SelectedFiles.IsEmpty())
	{
		return FReply::Handled();
	}

	int32 SucceededFiles = 0;
	int32 FailedFiles = 0;

	int64 TotalImportedCount = 0;
	int64 TotalSkippedCount = 0;

	for (const FString& FilePath : SelectedFiles)
	{
		int64 ImportedCount = 0;
		int64 SkippedCount = 0;

		if (!ActivityStorage.ImportRecordsFromJson( FilePath, ImportedCount, SkippedCount))
		{
			++FailedFiles;

			UE_LOG(LogProjectActivity, Error, TEXT("Failed to import JSON file: %s"), *FilePath);

			continue;
		}

		++SucceededFiles;

		TotalImportedCount += ImportedCount;
		TotalSkippedCount += SkippedCount;

		UE_LOG(LogProjectActivity, Log, TEXT("Activity import | New=%lld | Skipped=%lld | File=%s"), ImportedCount, SkippedCount, *FilePath);
	}

	RefreshDashboard();

	const FText Summary =
		FText::Format(
			LOCTEXT(
				"ActivityBatchImportSummary",
				"Files OK: {0}, failed: {1} | New: {2}, skipped: {3}"),
			FText::AsNumber(SucceededFiles),
			FText::AsNumber(FailedFiles),
			FText::AsNumber(TotalImportedCount),
			FText::AsNumber(TotalSkippedCount));

	if (PackageStatsStatusText.IsValid())
	{
		if (FailedFiles > 0)
		{
			PackageStatsStatusText->SetText(
				FText::Format(
					LOCTEXT(
						"ActivityBatchImportWithErrors",
						"{0} — See Output Log for failed files."),
					Summary));
		}
		else
		{
			PackageStatsStatusText->SetText(Summary);
		}
	}

	return FReply::Handled();
}

FReply FProjectActivityVisualizerModule::OnFindAssetClicked(FString PackagePath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> Assets;

	AssetRegistry.GetAssetsByPackageName(FName(*PackagePath) , Assets);

	if (Assets.IsEmpty())
	{
		const FText Message =
			AssetRegistry.IsLoadingAssets()
			? LOCTEXT(
				"AssetScanPending",
				"Asset discovery is still running. Please try again shortly.")
			: FText::Format(
				LOCTEXT(
					"RecordedAssetMissing",
					"No asset found at the recorded path: {0}"),
				FText::FromString(PackagePath));

		if (PackageStatsStatusText.IsValid())
		{
			PackageStatsStatusText->SetText(Message);
		}

		UE_LOG(LogProjectActivity , Warning , TEXT("Could not locate an asset for recorded package: %s") , *PackagePath);

		return FReply::Handled();
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	ContentBrowserModule.Get().SyncBrowserToAssets(Assets , false , true);

	if (PackageStatsStatusText.IsValid())
	{
		PackageStatsStatusText->SetText(FText::Format(LOCTEXT("RecordedAssetLocated","Located: {0}") , FText::FromString(PackagePath)));
	}

	UE_LOG(LogProjectActivity , Log , TEXT("Requested Content Browser selection for: %s") , *PackagePath);

	return FReply::Handled();
}

FReply FProjectActivityVisualizerModule::OnTimeRangeClicked(EProjectActivityTimePreset NewPreset)
{
	SelectedTimePreset = NewPreset;

	RefreshDashboard();

	return FReply::Handled();
}


void FProjectActivityVisualizerModule::BuildHotspotEvidence(const TArray<FProjectActivityBucketStats>& PackageStats, const FProjectActivityTimeRange& TimeRange, TArray<FProjectActivityHotspotEvidence>& OutEvidence)
{
	OutEvidence.Reset();

	if (PackageStats.IsEmpty())
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	const bool bCanReadReferences = !AssetRegistry.IsLoadingAssets();

	const auto CountProjectPackages = [](const TArray<FName>& Packages, const FName& OwnerPackage) -> int32
		{
			TSet<FName> UniquePackages;

			for (const FName& Name : Packages)
			{
				if (Name != OwnerPackage && Name.ToString().StartsWith(
						TEXT("/Game/"),
						ESearchCase::IgnoreCase))
				{
					UniquePackages.Add(Name);
				}
			}

			return UniquePackages.Num();
		};

	OutEvidence.Reserve(PackageStats.Num());

	for (const FProjectActivityBucketStats& Package : PackageStats)
	{
		FProjectActivityHotspotEvidence Evidence;

		Evidence.PackagePath = Package.PackagePath;
		Evidence.RawSaveCount = Package.RawSaveCount;
		Evidence.ActivityBucketCount = Package.ActivityBucketCount;

		TArray<FProjectActivityContributorStats> AliasStats;

		if (ActivityStorage.GetPackageContributorStats(
			Package.PackagePath,
			AliasStats,
			TimeRange))
		{
			Evidence.NamedAliasCount = 0;
			Evidence.UnspecifiedSaveCount = 0;

			for (const FProjectActivityContributorStats& Alias
				: AliasStats)
			{
				if (Alias.ContributorAlias.IsEmpty())
				{
					Evidence.UnspecifiedSaveCount += Alias.SaveCount;
				}
				else
				{
					// 조회 함수가 이미 별칭별로 묶어서 반환한다.
					++Evidence.NamedAliasCount;
				}
			}
		}

		if (bCanReadReferences)
		{
			const FName PackageName(*Package.PackagePath);

			TArray<FName> RawDependencies;
			TArray<FName> RawReferencers;

			if (AssetRegistry.GetDependencies(
				PackageName,
				RawDependencies,
				UE::AssetRegistry::EDependencyCategory::Package))
			{
				Evidence.GameDependencyCount =
					CountProjectPackages(
						RawDependencies,
						PackageName);
			}

			if (AssetRegistry.GetReferencers(
				PackageName,
				RawReferencers,
				UE::AssetRegistry::EDependencyCategory::Package))
			{
				Evidence.GameReferencerCount =
					CountProjectPackages(
						RawReferencers,
						PackageName);
			}
		}

		OutEvidence.Add(MoveTemp(Evidence));
	}
}

void FProjectActivityVisualizerModule::LogHotspotEvidence(const TArray<FProjectActivityHotspotEvidence>& EvidenceRows, const FProjectActivityTimeRange& TimeRange)
{
	UE_LOG(
		LogProjectActivity,
		Log,
		TEXT("Hotspot evidence summary | Range=%s | Sample=Top10ByActivityBuckets | Packages=%d | References=CurrentDirectGamePackages"),
		*MakeActivityTimeRangeText(TimeRange).ToString(),
		EvidenceRows.Num());

	const auto CountToText = [](int64 Count) -> FString
		{
			if (Count < 0)
			{
				return TEXT("Unknown");
			}

			return FString::Printf(TEXT("%lld"), Count);
		};

	for (const FProjectActivityHotspotEvidence& Evidence : EvidenceRows)
	{
		UE_LOG(
			LogProjectActivity,
			Log,
			TEXT("Hotspot evidence | Package=%s | Buckets5Min=%lld | RawSaves=%lld | NamedAliases=%s | UnspecifiedSaves=%s | GameDependencies=%s | GameReferencers=%s"),
			*Evidence.PackagePath,
			Evidence.ActivityBucketCount,
			Evidence.RawSaveCount,
			*CountToText(Evidence.NamedAliasCount),
			*CountToText(Evidence.UnspecifiedSaveCount),
			*CountToText(Evidence.GameDependencyCount),
			*CountToText(Evidence.GameReferencerCount));
	}
}

void FProjectActivityVisualizerModule::RefreshHotspotCandidates(const TArray<FProjectActivityHotspotEvidence>& EvidenceRows, const FProjectActivityTimeRange& TimeRange)
{
	if (!HotspotStatusText.IsValid()
		|| !HotspotDetailsText.IsValid())
	{
		return;
	}

	const UProjectActivitySettings* Settings = GetDefault<UProjectActivitySettings>();

	const TArray<FProjectActivityHotspotCandidate> Candidates = FProjectActivityAnalyzer::FindHotspotCandidates(EvidenceRows, *Settings);

	int32 IncompleteEvidenceCount = 0;

	for (const FProjectActivityHotspotEvidence& Evidence
		: EvidenceRows)
	{
		if (Evidence.NamedAliasCount < 0
			|| Evidence.GameReferencerCount < 0)
		{
			++IncompleteEvidenceCount;
		}
	}

	HotspotStatusText->SetText(
		FText::Format(
			LOCTEXT(
				"HotspotCandidateSummary",
				"{0} candidates / {1} packages checked | {2} | "
				"Packages with unavailable alias or referencer data: {3}"),
			FText::AsNumber(Candidates.Num()),
			FText::AsNumber(EvidenceRows.Num()),
			MakeActivityTimeRangeText(TimeRange), 
			FText::AsNumber(IncompleteEvidenceCount)));

	FString Details = FString::Printf(
		TEXT("Rules: activity buckets >= %d AND ")
		TEXT("(named aliases >= %d OR current /Game/ referencers >= %d).\n")
		TEXT("Activity and aliases use the selected time range. ")
		TEXT("Referencers use current saved package data.\n"),
		FMath::Max(1, Settings->HotspotMinActivityBuckets),
		FMath::Max(2, Settings->HotspotMinNamedAliases),
		FMath::Max(1, Settings->HotspotMinReferencers));

	const auto CountToText =
		[](int64 Count) -> FString
		{
			return Count < 0
				? FString(TEXT("Unknown"))
				: FString::Printf(TEXT("%lld"), Count);
		};

	for (const FProjectActivityHotspotCandidate& Candidate : Candidates)
	{
		TArray<FText> Reasons;

		if (Candidate.bMultipleAliasActivity)
		{
			Reasons.Add(
				LOCTEXT(
					"HotspotMultipleAliasReason",
					"Repeated activity across multiple aliases"));
		}

		if (Candidate.bReferencedActivity)
		{
			Reasons.Add(
				LOCTEXT(
					"HotspotReferencedAssetReason",
					"Repeated activity on an asset referenced by others"));
		}

		const FString ReasonText =
			FText::Join(
				LOCTEXT("HotspotReasonSeparator", "; "),
				Reasons).ToString();

		const FProjectActivityHotspotEvidence& Evidence =
			Candidate.Evidence;

		Details += FString::Printf(
			TEXT("\n%s\n")
			TEXT("Reason: %s\n")
			TEXT("Buckets5Min=%lld | RawSaves=%lld | ")
			TEXT("NamedAliases=%s | CurrentGameReferencers=%s\n"),
			*Evidence.PackagePath,
			*ReasonText,
			Evidence.ActivityBucketCount,
			Evidence.RawSaveCount,
			*CountToText(Evidence.NamedAliasCount),
			*CountToText(Evidence.GameReferencerCount));
	}

	if (Candidates.IsEmpty())
	{
		Details +=
			TEXT("\nNo candidates matched the rules with the available data.");
	}

	HotspotDetailsText->SetText(
		FText::FromString(Details));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProjectActivityVisualizerModule, ProjectActivityVisualizer)