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

DEFINE_LOG_CATEGORY_STATIC(LogProjectActivity, Log, All);

static const FName ProjectActivityVisualizerTabName("ProjectActivityVisualizer");

#define LOCTEXT_NAMESPACE "FProjectActivityVisualizerModule"

void FProjectActivityVisualizerModule::StartupModule()
{
	if (!ActivityStorage.Initialize())
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
}

void FProjectActivityVisualizerModule::ShutdownModule()
{
	if (PackageSavedDelegateHandle.IsValid())
	{
		UPackage::PackageSavedWithContextEvent.Remove(PackageSavedDelegateHandle);

		PackageSavedDelegateHandle.Reset();
	}

	if (TSharedPtr<SDockTab> OpenTab = DashboardTab.Pin())
	{
		OpenTab->RequestCloseTab();
	}

	DashboardTab.Reset();

	PackageStatsList.Reset();
	PackageStatsStatusText.Reset();
	RecordCountText.Reset();
	TimeRangeText.Reset();

	CategoryStatsList.Reset();
	CategoryStatsStatusText.Reset();

	DailyStatsList.Reset();
	DailyStatsStatusText.Reset();

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

TSharedRef<SDockTab>FProjectActivityVisualizerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
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
						.Padding(0.0f, 0.0f, 0.0f, 12.0f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT(
									"DashboardTitle",
									"Project Activity"))
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 12.0f)
						[
							SAssignNew(RecordCountText, STextBlock)
								.Text(LOCTEXT(
									"LoadingActivityCount",
									"Loading activity count..."))
								.AutoWrapText(true)
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
										.Text(LOCTEXT("AllTimeButton", "All time"))
										.OnClicked(
											FOnClicked::CreateRaw(
												this,
												&FProjectActivityVisualizerModule::
												OnTimeRangeClicked,
												false))
								]

							+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(8.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
										.Text(LOCTEXT("Last7DaysButton", "Last 7 days"))
										.OnClicked(
											FOnClicked::CreateRaw(
												this,
												&FProjectActivityVisualizerModule::
												OnTimeRangeClicked,
												true))
								]

							+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(8.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SButton)
										.Text(LOCTEXT("RefreshButtonLabel", "Refresh"))
										.OnClicked(
											FOnClicked::CreateRaw(
												this,
												&FProjectActivityVisualizerModule::
												OnRefreshClicked))
								]
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SAssignNew(TimeRangeText, STextBlock)
								.AutoWrapText(true)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 16.0f, 0.0f, 6.0f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT(
									"DailyStatsTitle",
									"Daily saves (last 7 UTC dates)"))
								.ToolTipText(LOCTEXT(
									"DailyStatsTitleTooltip",
									"Always shows today and the previous 6 UTC dates, "
									"independent of the time range buttons. "
									"Today includes saves up to the last refresh."))
								.AutoWrapText(true)
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SAssignNew(DailyStatsStatusText, STextBlock)
								.Text(LOCTEXT(
									"LoadingDailyStats",
									"Loading daily stats..."))
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
									"CategoryStatsTitle",
									"Saves by category"))
								.AutoWrapText(true)
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SAssignNew(CategoryStatsStatusText, STextBlock)
								.Text(LOCTEXT(
									"LoadingCategoryStats",
									"Loading category stats..."))
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
									"MostSavedPackagesTitle",
									"Most saved packages (all time, top 10)"))
								.AutoWrapText(true)
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SAssignNew(PackageStatsStatusText, STextBlock)
								.Text(LOCTEXT(
									"LoadingPackageStats",
									"Loading package stats..."))
								.AutoWrapText(true)
						]

					+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SAssignNew(PackageStatsList, SScrollBox)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 12.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT(
									"DashboardRefreshHint",
									"Save an asset, then click Refresh to update the count and list."))
								.AutoWrapText(true)
						]
				]
		];

	DashboardTab = NewTab;

	RefreshDashboard();

	return NewTab;
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

void FProjectActivityVisualizerModule::OnPackageSaved(const FString& PackageFileName,UPackage* Package,FObjectPostSaveContext SaveContext)
{
	// 패키지가 없으면 처리하지 않는다.
	if (Package == nullptr)
	{
		return;
	}

	// 쿠킹, 자동 처리, 자동 저장은 제외한다.
	if (SaveContext.IsCooking() || SaveContext.IsProceduralSave() || SaveContext.IsFromAutoSave())
	{
		return;
	}

	const FString PackagePath = Package->GetName();

	// 프로젝트 Content 아래의 패키지만 대상으로 한다.
	if (!PackagePath.StartsWith(TEXT("/Game/"),ESearchCase::CaseSensitive))
	{
		return;
	}

	// 활동 기록을 만들고 패키지 경로를 채운다.
	FProjectActivityRecord Record;
	Record.PackagePath = PackagePath;

	if (!ActivityStorage.InsertRecord(Record))
	{
		UE_LOG(LogProjectActivity , Error , TEXT("Activity could not be saved: %s") , *Record.PackagePath);

		return;
	}

	const UProjectActivitySettings* Settings = GetDefault<UProjectActivitySettings>();

	const FName Category = Settings->ResolveCategory(Record.PackagePath);

	UE_LOG(LogProjectActivity , Log , TEXT("Package category | Package=%s | Category=%s") , *Record.PackagePath , *Category.ToString());

	// 로그에 표시할 문자열로 변환한다.
	const FString EventIdText = Record.EventId.ToString(EGuidFormats::DigitsWithHyphens);

	const FString TimestampText = Record.TimestampUtc.ToIso8601();

	UE_LOG(LogProjectActivity,Log,TEXT("Activity saved to SQLite | Id=%s | Package=%s | Utc=%s | Type=%d") , *EventIdText , *Record.PackagePath , *TimestampText , static_cast<int32>(Record.ActivityType));
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
		|| !DailyStatsStatusText.IsValid())
	{
		return;
	}

	// 이전에 표시한 행을 제거한다.
	PackageStatsList->ClearChildren();
	FProjectActivityTimeRange TimeRange;
	TimeRange.bEnabled = bUseLast7Days;

	if (TimeRange.bEnabled)
	{
		TimeRange.EndUtc = FDateTime::UtcNow();

		TimeRange.StartUtc = TimeRange.EndUtc - FTimespan::FromDays(7.0);
	}

	TimeRangeText->SetText(
		bUseLast7Days
		? LOCTEXT("Last7DaysRangeLabel", "Time range: Last 7 days (rolling 168 hours)")
		: LOCTEXT("AllTimeRangeLabel", "Time range: All time"));

	RefreshCategoryStats(TimeRange);
	RefreshDailyStats();

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

	TArray<FProjectActivityPackageStats> TopPackages;

	if (!ActivityStorage.GetMostSavedPackages(10, TopPackages, TimeRange))
	{
		PackageStatsStatusText->SetText(
			LOCTEXT(
				"PackageStatsError",
				"Could not read package stats. See Output Log."));

		UE_LOG(LogProjectActivity , Error , TEXT("Dashboard failed to read package stats."));

		return;
	}

	if (TopPackages.IsEmpty())
	{
		PackageStatsStatusText->SetText(
			LOCTEXT(
				"NoPackageStats",
				"No save events recorded yet."));

		return;
	}

	PackageStatsStatusText->SetText(
		FText::Format(
			LOCTEXT(
				"PackageStatsResultCount",
				"Showing {0} packages"),
			FText::AsNumber(TopPackages.Num())));

	for (const FProjectActivityPackageStats& Stats : TopPackages)
	{
		PackageStatsList->AddSlot()
			.Padding(0.0f, 3.0f)
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
								SNew(STextBlock)
									.Text(
										FText::Format(
											LOCTEXT(
												"PackageSaveCount",
												"{0} saves"),
											FText::AsNumber(Stats.SaveCount)))
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
					]
			];
	}

}

void FProjectActivityVisualizerModule::RefreshCategoryStats(const FProjectActivityTimeRange& TimeRange)
{
	if (!CategoryStatsList.IsValid() || !CategoryStatsStatusText.IsValid())
	{
		return;
	}

	// 새 결과를 표시하기 전에 이전 행을 제거한다.
	CategoryStatsList->ClearChildren();

	TArray<FProjectActivityPackageStats> AllPackageStats;

	if (!ActivityStorage.GetPackageSaveCounts(AllPackageStats , TimeRange))
	{
		CategoryStatsStatusText->SetText(
			LOCTEXT(
				"CategoryStatsQueryError",
				"Could not read category stats. See Output Log."));

		UE_LOG(LogProjectActivity , Error , TEXT("Dashboard failed to read category source data."));

		return;
	}

	const UProjectActivitySettings* Settings = GetDefault<UProjectActivitySettings>();

	const TArray<FProjectActivityCategoryStats> CategoryStats = FProjectActivityAnalyzer::SummarizeCategories(AllPackageStats, *Settings);

	int64 TotalSaves = 0;

	for (const FProjectActivityCategoryStats& Stats : CategoryStats)
	{
		TotalSaves += Stats.SaveCount;
	}

	if (TotalSaves <= 0)
	{
		CategoryStatsStatusText->SetText(
			LOCTEXT(
				"NoCategoryStats",
				"No save events in the selected time range."));

		return;
	}

	CategoryStatsStatusText->SetText(
		FText::Format(
			LOCTEXT(
				"CategoryStatsSummary",
				"{0} categories / {1} saves in the selected time range"),
			FText::AsNumber(CategoryStats.Num()),
			FText::AsNumber(TotalSaves)));


	FNumberFormattingOptions PercentOptions;
	PercentOptions.MinimumFractionalDigits = 1;
	PercentOptions.MaximumFractionalDigits = 1;

	for (const FProjectActivityCategoryStats& Stats : CategoryStats)
	{
		const double Share =
			static_cast<double>(Stats.SaveCount)
			/ static_cast<double>(TotalSaves);

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
														"CategorySaveCountAndShare",
														"{0} saves ({1})"),
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

		UE_LOG(LogProjectActivity , Log , TEXT("Category stats | Category=%s | Saves=%lld") , *Stats.CategoryName.ToString() , Stats.SaveCount);
	}
}

void FProjectActivityVisualizerModule::RefreshDailyStats()
{
	if (!DailyStatsList.IsValid() || !DailyStatsStatusText.IsValid())
	{
		return;
	}

	// 이전 그래프 행을 제거한다.
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
	int64 MaxSaves = 0;

	for (const FProjectActivityDailyStats& Stats : DailyStats)
	{
		TotalSaves += Stats.SaveCount;

		MaxSaves = FMath::Max(
			MaxSaves,
			Stats.SaveCount);
	}

	if (TotalSaves == 0)
	{
		DailyStatsStatusText->SetText(
			LOCTEXT(
				"DailyStatsNoSaves",
				"No saves in these 7 UTC dates. Today is partial."));
	}
	else
	{
		DailyStatsStatusText->SetText(
			FText::Format(
				LOCTEXT(
					"DailyStatsSummary",
					"{0} to {1} (UTC) / {2} saves / "
					"Full bar: {3} saves. Today is partial."),
				FText::FromString(DailyStats[0].DateUtc),
				FText::FromString(DailyStats.Last().DateUtc),
				FText::AsNumber(TotalSaves),
				FText::AsNumber(MaxSaves)));
	}

	for (const FProjectActivityDailyStats& Stats : DailyStats)
	{
		
		const float BarFraction =
			MaxSaves > 0
			? static_cast<float>(
				static_cast<double>(Stats.SaveCount)
				/ static_cast<double>(MaxSaves))
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
									.WidthOverride(90.0f)
									.HAlign(HAlign_Right)
									[
										SNew(STextBlock)
											.Text(
												FText::Format(
													LOCTEXT(
														"DailySaveCount",
														"{0} saves"),
													FText::AsNumber(
														Stats.SaveCount)))
									]
							]
					]
			];

		UE_LOG(LogProjectActivity, Log, TEXT("Daily stats | DateUtc=%s | Saves=%lld"), *Stats.DateUtc, Stats.SaveCount);
	}
}

FReply FProjectActivityVisualizerModule::OnRefreshClicked()
{
	RefreshDashboard();

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

FReply FProjectActivityVisualizerModule::OnTimeRangeClicked(
	bool bNewUseLast7Days)
{
	bUseLast7Days = bNewUseLast7Days;

	RefreshDashboard();

	return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProjectActivityVisualizerModule, ProjectActivityVisualizer)