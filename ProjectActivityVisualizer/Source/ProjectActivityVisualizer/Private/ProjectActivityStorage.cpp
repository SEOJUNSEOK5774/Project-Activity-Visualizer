#include "ProjectActivityStorage.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "SQLitePreparedStatement.h"
#include "Misc/Timespan.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Misc/ScopeExit.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectActivityStorage, Log, All);

static bool BindActivityTimeRange(FSQLitePreparedStatement& Statement , int32 FirstBindingIndex , const FProjectActivityTimeRange& TimeRange);

FProjectActivityStorage::~FProjectActivityStorage()
{
    Close();
}

bool FProjectActivityStorage::EnsureContributorAliasColumn()
{
    int64 ColumnCount = 0;

    {
        FSQLitePreparedStatement Statement;

        const TCHAR* QuerySql =
            TEXT("SELECT COUNT(*) FROM pragma_table_info('Activity') ")
            TEXT("WHERE name = 'ContributorAlias' COLLATE NOCASE;");

        if (!Statement.Create(Database, QuerySql) || Statement.Step() != ESQLitePreparedStatementStepResult::Row || !Statement.GetColumnValueByIndex(0, ColumnCount))
        {
            UE_LOG(LogProjectActivityStorage , Error , TEXT("Failed to inspect Activity schema: %s") , *Database.GetLastError());

            return false;
        }
    }

    // 이미 컬럼이 있으면 다시 추가하지 않는다.
    if (ColumnCount > 0)
    {
        return true;
    }

    const TCHAR* AlterSql =
        TEXT("ALTER TABLE Activity ")
        TEXT("ADD COLUMN ContributorAlias TEXT NOT NULL DEFAULT '';");

    if (!Database.Execute(AlterSql))
    {
        UE_LOG(LogProjectActivityStorage , Error , TEXT("Failed to add ContributorAlias column: %s") , *Database.GetLastError());

        return false;
    }

    UE_LOG(LogProjectActivityStorage , Log , TEXT("Added ContributorAlias column to Activity."));

    return true;
}

bool FProjectActivityStorage::EnsureSourceControlSnapshotCacheTable()
{
    if (!Database.IsValid())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot prepare source control snapshot cache: database is not open."));

        return false;
    }

    const TCHAR* CreateTableSql =
        TEXT("CREATE TABLE IF NOT EXISTS SourceControlSnapshotCache (")
        TEXT("PackagePath TEXT PRIMARY KEY NOT NULL,")
        TEXT("ProviderName TEXT NOT NULL,")
        TEXT("RepositoryRoot TEXT NOT NULL,")
        TEXT("BranchName TEXT NOT NULL,")
        TEXT("FetchedAtUtc TEXT NOT NULL,")
        TEXT("SnapshotJson TEXT NOT NULL")
        TEXT(");");

    if (!Database.Execute(CreateTableSql))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to prepare source control snapshot cache: %s"), *Database.GetLastError());

        return false;
    }

    UE_LOG(LogProjectActivityStorage, Log, TEXT("Source control snapshot cache table ready."));

    return true;
}

bool FProjectActivityStorage::Initialize()
{
    if (Database.IsValid())
    {
        return true;
    }

    const FString StorageDirectory =FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ProjectActivity")));

    if (!IFileManager::Get().MakeDirectory(*StorageDirectory, true))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to create storage directory: %s"), *StorageDirectory);

        return false;
    }

    const FString DatabasePath = FPaths::Combine(StorageDirectory, TEXT("activity.db"));

    if (!Database.Open(*DatabasePath, ESQLiteDatabaseOpenMode::ReadWriteCreate))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to open activity database: %s"), *DatabasePath);

        return false;
    }

    const TCHAR* CreateTableSql =
        TEXT("CREATE TABLE IF NOT EXISTS Activity (")
        TEXT("EventId TEXT PRIMARY KEY NOT NULL,")
        TEXT("PackagePath TEXT NOT NULL,")
        TEXT("TimestampUtc TEXT NOT NULL,")
        TEXT("ActivityType INTEGER NOT NULL,")
        TEXT("ContributorAlias TEXT NOT NULL DEFAULT ''")
        TEXT(");");

    if (!Database.Execute(CreateTableSql))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to create Activity table: %s"), *Database.GetLastError());

        Close();
        return false;
    }

    if (!EnsureContributorAliasColumn())
    {
        Close();
        return false;
    }

    if (!EnsureSourceControlSnapshotCacheTable())
    {
        Close();
        return false;
    }

    UE_LOG(LogProjectActivityStorage, Log, TEXT("Activity database ready: %s"), *DatabasePath);

    return true;
}
bool FProjectActivityStorage::InsertRecord(const FProjectActivityRecord& Record)
{
    if (!Database.IsValid())
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Cannot insert activity: database is not open."));

        return false;
    }

    const TCHAR* InsertSql =
        TEXT("INSERT INTO Activity ")
        TEXT("(EventId, PackagePath, TimestampUtc, ActivityType, ContributorAlias) ")
        TEXT("VALUES (?1, ?2, ?3, ?4, ?5);");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, InsertSql))
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Failed to prepare activity insert: %s"), *Database.GetLastError());

        return false;
    }

    const FString EventIdText = Record.EventId.ToString(EGuidFormats::DigitsWithHyphens);

    const FString TimestampText = Record.TimestampUtc.ToIso8601();

    const bool bBindingsSucceeded =
        Statement.SetBindingValueByIndex(1, EventIdText)
        && Statement.SetBindingValueByIndex(2, Record.PackagePath)
        && Statement.SetBindingValueByIndex(3, TimestampText)
        && Statement.SetBindingValueByIndex(
            4,
            static_cast<int32>(Record.ActivityType))
        && Statement.SetBindingValueByIndex(
            5,
            Record.ContributorAlias);

    if (!bBindingsSucceeded)
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Failed to bind activity values: %s"), *Database.GetLastError());

        return false;
    }

    if (!Statement.Execute())
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Failed to insert activity: %s"), *Database.GetLastError());

        return false;
    }

    return true;
}

bool FProjectActivityStorage::ExportAllRecordsToJson(const FString& FilePath, int64& OutRecordCount)
{
    OutRecordCount = 0;

    if (!Database.IsValid())
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Cannot export activity: database is not open."));

        return false;
    }

    const TCHAR* QuerySql =
        TEXT("SELECT EventId, PackagePath, ContributorAlias, ")
        TEXT("TimestampUtc, ActivityType ")
        TEXT("FROM Activity ")
        TEXT("ORDER BY TimestampUtc ASC, EventId ASC;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to prepare activity export: %s"), *Database.GetLastError());

        return false;
    }

    TArray<TSharedPtr<FJsonValue>> JsonRecords;

    while (true)
    {
        const ESQLitePreparedStatementStepResult StepResult = Statement.Step();

        if (StepResult == ESQLitePreparedStatementStepResult::Done)
        {
            break;
        }

        if (StepResult != ESQLitePreparedStatementStepResult::Row)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read activity for export: %s"), *Database.GetLastError());

            return false;
        }

        FString EventId;
        FString PackagePath;
        FString ContributorAlias;
        FString TimestampUtc;
        int32 ActivityType = 0;

        const bool bReadSucceeded =
            Statement.GetColumnValueByIndex(0, EventId)
            && Statement.GetColumnValueByIndex(1, PackagePath)
            && Statement.GetColumnValueByIndex(2, ContributorAlias)
            && Statement.GetColumnValueByIndex(3, TimestampUtc)
            && Statement.GetColumnValueByIndex(4, ActivityType);

        if (!bReadSucceeded)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read an activity export row."));

            return false;
        }

        TSharedRef<FJsonObject> RecordObject = MakeShared<FJsonObject>();

        RecordObject->SetStringField(TEXT("EventId"), EventId);

        RecordObject->SetStringField(TEXT("PackagePath"), PackagePath);

        RecordObject->SetStringField(TEXT("ContributorAlias"), ContributorAlias);

        RecordObject->SetStringField(TEXT("TimestampUtc"), TimestampUtc);

        RecordObject->SetNumberField(TEXT("ActivityType"), ActivityType);

        JsonRecords.Add(MakeShared<FJsonValueObject>(RecordObject));
    }

    TSharedRef<FJsonObject> RootObject =
        MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("Format") , TEXT("ProjectActivityVisualizer"));

    RootObject->SetNumberField(TEXT("SchemaVersion"), 1);

    RootObject->SetStringField(TEXT("ProjectName"), FApp::GetProjectName());

    RootObject->SetStringField(TEXT("ExportedAtUtc"), FDateTime::UtcNow().ToIso8601());

    RootObject->SetNumberField(TEXT("RecordCount"), JsonRecords.Num());

    RootObject->SetArrayField(TEXT("Records"), JsonRecords);

    FString JsonText;

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);

    if (!FJsonSerializer::Serialize(RootObject, Writer))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to serialize activity JSON."));

        return false;
    }

    const FString Directory = FPaths::GetPath(FilePath);

    if (!IFileManager::Get().MakeDirectory(*Directory, true))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to create export directory: %s"), *Directory);

        return false;
    }

    if (!FFileHelper::SaveStringToFile(JsonText , *FilePath , FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to write activity JSON: %s"), *FilePath);

        return false;
    }

    OutRecordCount = JsonRecords.Num();

    return true;
}

bool FProjectActivityStorage::ImportRecordsFromJson(const FString& FilePath, int64& OutImportedCount, int64& OutSkippedCount)
{
    OutImportedCount = 0;
    OutSkippedCount = 0;

    if (!Database.IsValid())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot import activity: database is not open."));

        return false;
    }

    FString JsonText;

    if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read activity JSON: %s"), *FilePath);

        return false;
    }

    TSharedPtr<FJsonObject> RootObject;

    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);

    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Invalid activity JSON."));

        return false;
    }

    FString Format;
    FString ProjectName;
    double SchemaVersion = 0;
    double DeclaredRecordCount = 0;

    const TArray<TSharedPtr<FJsonValue>>* JsonRecords = nullptr;

    const bool bHeaderValid =
        RootObject->TryGetStringField(TEXT("Format"), Format)
        && RootObject->TryGetNumberField(
            TEXT("SchemaVersion"), SchemaVersion)
        && RootObject->TryGetStringField(
            TEXT("ProjectName"), ProjectName)
        && RootObject->TryGetNumberField(
            TEXT("RecordCount"), DeclaredRecordCount)
        && RootObject->TryGetArrayField(
            TEXT("Records"), JsonRecords);

    if (!bHeaderValid
        || JsonRecords == nullptr
        || Format != TEXT("ProjectActivityVisualizer")
        || SchemaVersion != 1.0
        || !ProjectName.Equals(
            FApp::GetProjectName(),
            ESearchCase::CaseSensitive)
        || DeclaredRecordCount !=
        static_cast<double>(JsonRecords->Num()))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Activity JSON format, version, project name, or record count did not match."));

        return false;
    }

    // 모든 기록을 검증한 뒤 DB 저장을 시작한다.
    TArray<FProjectActivityRecord> Records;
    Records.Reserve(JsonRecords->Num());

    for (int32 Index = 0; Index < JsonRecords->Num(); ++Index)
    {
        const TSharedPtr<FJsonValue>& Value = (*JsonRecords)[Index];

        if (!Value.IsValid() || Value->Type != EJson::Object)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Invalid activity object at record %d."), Index + 1);

            return false;
        }

        const TSharedPtr<FJsonObject> Object = Value->AsObject();

        FProjectActivityRecord Record;
        FString EventIdText;
        FString TimestampText;
        double ActivityTypeNumber = 0;

        const bool bFieldsValid =
            Object.IsValid()
            && Object->TryGetStringField(
                TEXT("EventId"), EventIdText)
            && Object->TryGetStringField(
                TEXT("PackagePath"), Record.PackagePath)
            && Object->TryGetStringField(
                TEXT("ContributorAlias"), Record.ContributorAlias)
            && Object->TryGetStringField(
                TEXT("TimestampUtc"), TimestampText)
            && Object->TryGetNumberField(
                TEXT("ActivityType"), ActivityTypeNumber);

        if (!bFieldsValid
            || !FGuid::ParseExact(
                EventIdText,
                EGuidFormats::DigitsWithHyphens,
                Record.EventId)
            || !Record.EventId.IsValid()
            || !TimestampText.EndsWith(
                TEXT("Z"), ESearchCase::CaseSensitive)
            || !FDateTime::ParseIso8601(
                *TimestampText, Record.TimestampUtc)
            || !Record.PackagePath.StartsWith(
                TEXT("/Game/"), ESearchCase::CaseSensitive)
            || Record.PackagePath.Len() <= 6
            || ActivityTypeNumber !=
            static_cast<double>(
                EProjectActivityType::PackageSaved))
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Invalid or unsupported activity data at record %d."), Index + 1);

            return false;
        }

        Record.ActivityType = EProjectActivityType::PackageSaved;

        Records.Add(MoveTemp(Record));
    }

    if (!Database.Execute(TEXT("BEGIN IMMEDIATE TRANSACTION;")))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to begin activity import: %s"), *Database.GetLastError());

        return false;
    }

    bool bCommitted = false;

    ON_SCOPE_EXIT
    {
        if (!bCommitted && !Database.Execute(TEXT("ROLLBACK;")))
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to roll back activity import: %s"), *Database.GetLastError());

            Close();
        }
    };

    int64 ImportedCount = 0;
    int64 SkippedCount = 0;

    const TCHAR* ExistsSql =
        TEXT("SELECT COUNT(*) FROM Activity WHERE EventId = ?1;");

    for (const FProjectActivityRecord& Record : Records)
    {
        const FString EventIdText = Record.EventId.ToString(EGuidFormats::DigitsWithHyphens);

        int64 ExistingCount = 0;

        {
            FSQLitePreparedStatement ExistsStatement;

            if (!ExistsStatement.Create(Database, ExistsSql)
                || !ExistsStatement.SetBindingValueByIndex(
                    1, EventIdText)
                || ExistsStatement.Step() !=
                ESQLitePreparedStatementStepResult::Row
                || !ExistsStatement.GetColumnValueByIndex(
                    0, ExistingCount))
            {
                UE_LOG(
                    LogProjectActivityStorage,
                    Error,
                    TEXT("Failed to check an imported EventId: %s"),
                    *Database.GetLastError());

                return false;
            }
        }

        if (ExistingCount > 0)
        {
            ++SkippedCount;
            continue;
        }

        if (!InsertRecord(Record))
        {
            return false;
        }

        ++ImportedCount;
    }

    if (!Database.Execute(TEXT("COMMIT;")))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to commit activity import: %s"), *Database.GetLastError());

        return false;
    }

    bCommitted = true;

    OutImportedCount = ImportedCount;
    OutSkippedCount = SkippedCount;

    return true;
}

bool FProjectActivityStorage::GetRecordCount(int64& OutCount , const FProjectActivityTimeRange& TimeRange)
{
    OutCount = 0;

    if (!Database.IsValid())
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Cannot count activity: database is not open."));

        return false;
    }

    const TCHAR* QuerySql =
        TEXT("SELECT COUNT(*) FROM Activity ")
        TEXT("WHERE (?1 = 0 OR ")
        TEXT("(TimestampUtc >= ?2 AND TimestampUtc <= ?3));");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql))
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Failed to prepare activity count: %s"), *Database.GetLastError());

        return false;
    }

    if (!BindActivityTimeRange(Statement, 1, TimeRange))
    {
        UE_LOG(LogProjectActivityStorage , Error , TEXT("Failed to bind activity count time range."));

        return false;
    }

    if (Statement.Step() != ESQLitePreparedStatementStepResult::Row)
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Failed to query activity count: %s"), *Database.GetLastError());

        return false;
    }

    if (!Statement.GetColumnValueByIndex(0, OutCount))
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Failed to read activity count."));

        return false;
    }

    return true;
}

bool FProjectActivityStorage::GetMostSavedPackages(int32 Limit , TArray<FProjectActivityPackageStats>& OutStats , const FProjectActivityTimeRange& TimeRange)
{
    OutStats.Reset();

    if (!Database.IsValid())
    {
        UE_LOG(LogProjectActivityStorage , Error , TEXT("Cannot query package stats: database is not open."));

        return false;
    }

    if (Limit <= 0)
    {
        UE_LOG( LogProjectActivityStorage , Error , TEXT("Package stats limit must be greater than zero."));

        return false;
    }

    const TCHAR* QuerySql =
        TEXT("SELECT PackagePath, COUNT(*) AS SaveCount ")
        TEXT("FROM Activity ")
        TEXT("WHERE ActivityType = ?1 ")
        TEXT("AND (?2 = 0 OR ")
        TEXT("(TimestampUtc >= ?3 AND TimestampUtc <= ?4)) ")
        TEXT("GROUP BY PackagePath ")
        TEXT("ORDER BY SaveCount DESC, PackagePath ASC ")
        TEXT("LIMIT ?5;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql))
    {
        UE_LOG( LogProjectActivityStorage , Error , TEXT("Failed to prepare package stats query: %s") , *Database.GetLastError());

        return false;
    }

    const bool bBindingsSucceeded =Statement.SetBindingValueByIndex(1 , static_cast<int32>(EProjectActivityType::PackageSaved)) && BindActivityTimeRange(Statement, 2, TimeRange) && Statement.SetBindingValueByIndex(5, Limit);

    if (!bBindingsSucceeded)
    {
        UE_LOG( LogProjectActivityStorage , Error , TEXT("Failed to bind package stats query: %s") , *Database.GetLastError());

        return false;
    }

    TArray<FProjectActivityPackageStats> Results;

    while (true)
    {
        const ESQLitePreparedStatementStepResult StepResult =
            Statement.Step();

        if (StepResult == ESQLitePreparedStatementStepResult::Done)
        {
            break;
        }

        if (StepResult != ESQLitePreparedStatementStepResult::Row)
        {
            UE_LOG( LogProjectActivityStorage , Error , TEXT("Failed to query package stats: %s") , *Database.GetLastError());

            return false;
        }

        FProjectActivityPackageStats Stats;

        if (!Statement.GetColumnValueByIndex(0, Stats.PackagePath)
            || !Statement.GetColumnValueByIndex(1, Stats.SaveCount))
        {
            UE_LOG( LogProjectActivityStorage , Error , TEXT("Failed to read package stats row."));

            return false;
        }

        Results.Add(MoveTemp(Stats));
    }

    OutStats = MoveTemp(Results);

    return true;
}

bool FProjectActivityStorage::GetPackageSaveCounts(TArray<FProjectActivityPackageStats>& OutStats , const FProjectActivityTimeRange& TimeRange)
{
    OutStats.Reset();

    if (!Database.IsValid())
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Cannot query package save counts: database is not open."));

        return false;
    }

    const TCHAR* QuerySql =
        TEXT("SELECT PackagePath, COUNT(*) AS SaveCount ")
        TEXT("FROM Activity ")
        TEXT("WHERE ActivityType = ?1 ")
        TEXT("AND (?2 = 0 OR ")
        TEXT("(TimestampUtc >= ?3 AND TimestampUtc <= ?4)) ")
        TEXT("GROUP BY PackagePath ")
        TEXT("ORDER BY SaveCount DESC, PackagePath ASC;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql))
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Failed to prepare package save counts: %s"), *Database.GetLastError());

        return false;
    }

    const bool bBindingsSucceeded =
        Statement.SetBindingValueByIndex(1, static_cast<int32>(EProjectActivityType::PackageSaved)) && BindActivityTimeRange(Statement, 2, TimeRange);

    if (!bBindingsSucceeded)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to bind package save counts: %s"), *Database.GetLastError());

        return false;
    }

    TArray<FProjectActivityPackageStats> Results;

    while (true)
    {
        const ESQLitePreparedStatementStepResult StepResult =
            Statement.Step();

        if (StepResult == ESQLitePreparedStatementStepResult::Done)
        {
            break;
        }

        if (StepResult != ESQLitePreparedStatementStepResult::Row)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to query package save counts: %s"), *Database.GetLastError());

            return false;
        }

        FProjectActivityPackageStats Stats;

        if (!Statement.GetColumnValueByIndex(0, Stats.PackagePath)
            || !Statement.GetColumnValueByIndex(1, Stats.SaveCount))
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read package save counts row."));

            return false;
        }

        Results.Add(MoveTemp(Stats));
    }

    OutStats = MoveTemp(Results);

    return true;
}

bool FProjectActivityStorage::GetRecentDailySaveCounts(int32 NumDays, TArray<FProjectActivityDailyStats>& OutStats, const FDateTime& NowUtc)
{
    OutStats.Reset();

    if (!Database.IsValid())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot query daily stats: database is not open."));

        return false;
    }

    if (NumDays < 1 || NumDays > 366)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Daily stats day count must be between 1 and 366."));

        return false;
    }

    const FDateTime FirstDayUtc = NowUtc.GetDate() - FTimespan::FromDays(static_cast<double>(NumDays - 1));

    const TCHAR* QuerySql =
        TEXT("WITH BucketStats AS (")
        TEXT("SELECT substr(TimestampUtc, 1, 10) AS DayUtc, ")
        TEXT("PackagePath, TRIM(ContributorAlias) AS Alias, ")
        TEXT("CAST(strftime('%s', TimestampUtc) AS INTEGER) / 300 ")
        TEXT("AS BucketId, COUNT(*) AS RawSaveCount ")
        TEXT("FROM Activity ")
        TEXT("WHERE ActivityType = ?1 ")
        TEXT("AND TimestampUtc >= ?2 ")
        TEXT("AND TimestampUtc <= ?3 ")
        TEXT("GROUP BY DayUtc, PackagePath, Alias, BucketId")
        TEXT(") ")
        TEXT("SELECT DayUtc, SUM(RawSaveCount) AS SaveCount, ")
        TEXT("COUNT(*) AS ActivityBucketCount ")
        TEXT("FROM BucketStats ")
        TEXT("GROUP BY DayUtc ")
        TEXT("ORDER BY DayUtc ASC;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to prepare daily stats: %s"), *Database.GetLastError());

        return false;
    }

    const bool bBindingsSucceeded =
        Statement.SetBindingValueByIndex(
            1,
            static_cast<int32>(
                EProjectActivityType::PackageSaved))
        && Statement.SetBindingValueByIndex(
            2,
            FirstDayUtc.ToIso8601())
        && Statement.SetBindingValueByIndex(
            3,
            NowUtc.ToIso8601());

    if (!bBindingsSucceeded)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to bind daily stats: %s"), *Database.GetLastError());

        return false;
    }

    TMap<FString, FProjectActivityDailyStats> CountsByDay;

    while (true)
    {
        const ESQLitePreparedStatementStepResult StepResult = Statement.Step();

        if (StepResult == ESQLitePreparedStatementStepResult::Done)
        {
            break;
        }

        if (StepResult != ESQLitePreparedStatementStepResult::Row)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to query daily stats: %s"), *Database.GetLastError());

            return false;
        }

        FProjectActivityDailyStats Stats;

        const bool bReadSucceeded =
            Statement.GetColumnValueByIndex(
                0,
                Stats.DateUtc)
            && Statement.GetColumnValueByIndex(
                1,
                Stats.SaveCount)
            && Statement.GetColumnValueByIndex(
                2,
                Stats.ActivityBucketCount);

        if (!bReadSucceeded)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read daily stats row."));

            return false;
        }

        CountsByDay.Add(Stats.DateUtc, Stats);
    }

    // 기록이 없는 날짜도 기본값 0으로 포함한다.
    TArray<FProjectActivityDailyStats> Results;
    Results.Reserve(NumDays);

    for (int32 DayIndex = 0; DayIndex < NumDays; ++DayIndex)
    {
        const FDateTime DayUtc =
            FirstDayUtc
            + FTimespan::FromDays(
                static_cast<double>(DayIndex));

        FProjectActivityDailyStats Stats;
        Stats.DateUtc = DayUtc.ToString(TEXT("%Y-%m-%d"));

        const FProjectActivityDailyStats* Found =
            CountsByDay.Find(Stats.DateUtc);

        if (Found != nullptr)
        {
            Stats.SaveCount = Found->SaveCount;
            Stats.ActivityBucketCount = Found->ActivityBucketCount;
        }

        Results.Add(MoveTemp(Stats));
    }

    OutStats = MoveTemp(Results);

    return true;
}

bool FProjectActivityStorage::GetMultiContributorPackages(int32 Limit, TArray<FProjectActivityMultiContributorStats>& OutStats, const FProjectActivityTimeRange& TimeRange)
{
    OutStats.Reset();

    if (!Database.IsValid())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot query multi-contributor stats: database is not open."));

        return false;
    }

    if (Limit <= 0)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Multi-contributor stats limit must be greater than zero."));

        return false;
    }

    const TCHAR* QuerySql =
        TEXT("SELECT PackagePath, COUNT(*) AS SaveCount, ")
        TEXT("COUNT(DISTINCT NULLIF(TRIM(ContributorAlias), '')) ")
        TEXT("AS ContributorCount ")
        TEXT("FROM Activity ")
        TEXT("WHERE ActivityType = ?1 ")
        TEXT("AND (?2 = 0 OR ")
        TEXT("(TimestampUtc >= ?3 AND TimestampUtc <= ?4)) ")
        TEXT("GROUP BY PackagePath ")
        TEXT("HAVING COUNT(DISTINCT ")
        TEXT("NULLIF(TRIM(ContributorAlias), '')) >= 2 ")
        TEXT("ORDER BY ContributorCount DESC, ")
        TEXT("SaveCount DESC, PackagePath ASC ")
        TEXT("LIMIT ?5;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to prepare multi-contributor stats: %s"), *Database.GetLastError());

        return false;
    }

    const bool bBindingsSucceeded =
        Statement.SetBindingValueByIndex(1, static_cast<int32>(EProjectActivityType::PackageSaved))
        && BindActivityTimeRange(Statement, 2, TimeRange)
        && Statement.SetBindingValueByIndex(5, Limit);

    if (!bBindingsSucceeded)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to bind multi-contributor stats: %s"), *Database.GetLastError());

        return false;
    }

    TArray<FProjectActivityMultiContributorStats> Results;

    while (true)
    {
        const ESQLitePreparedStatementStepResult StepResult = Statement.Step();

        if (StepResult == ESQLitePreparedStatementStepResult::Done)
        {
            break;
        }

        if (StepResult != ESQLitePreparedStatementStepResult::Row)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to query multi-contributor stats: %s"), *Database.GetLastError());

            return false;
        }

        FProjectActivityMultiContributorStats Stats;

        const bool bReadSucceeded = Statement.GetColumnValueByIndex( 0, Stats.PackagePath)
            && Statement.GetColumnValueByIndex(1, Stats.SaveCount)
            && Statement.GetColumnValueByIndex(2, Stats.ContributorCount);

        if (!bReadSucceeded)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read multi-contributor stats row."));

            return false;
        }

        Results.Add(MoveTemp(Stats));
    }

    OutStats = MoveTemp(Results);

    return true;
}

bool FProjectActivityStorage::GetPackageContributorStats(const FString& PackagePath, TArray<FProjectActivityContributorStats>& OutStats, const FProjectActivityTimeRange& TimeRange)
{
    OutStats.Reset();

    if (!Database.IsValid())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot query contributor details: database is not open."));

        return false;
    }

    if (PackagePath.IsEmpty())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot query contributor details: package path is empty."));

        return false;
    }

    const TCHAR* QuerySql =
        TEXT("SELECT TRIM(ContributorAlias) AS Alias, ")
        TEXT("COUNT(*) AS SaveCount ")
        TEXT("FROM Activity ")
        TEXT("WHERE PackagePath = ?1 ")
        TEXT("AND ActivityType = ?2 ")
        TEXT("AND (?3 = 0 OR ")
        TEXT("(TimestampUtc >= ?4 AND TimestampUtc <= ?5)) ")
        TEXT("GROUP BY TRIM(ContributorAlias) ")
        TEXT("ORDER BY SaveCount DESC, Alias ASC;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to prepare contributor details: %s"), *Database.GetLastError());

        return false;
    }

    const bool bBindingsSucceeded =Statement.SetBindingValueByIndex(1, PackagePath)
        && Statement.SetBindingValueByIndex(2, static_cast<int32>(EProjectActivityType::PackageSaved))
        && BindActivityTimeRange(Statement, 3, TimeRange);

    if (!bBindingsSucceeded)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to bind contributor details: %s"), *Database.GetLastError());

        return false;
    }

    TArray<FProjectActivityContributorStats> Results;

    while (true)
    {
        const ESQLitePreparedStatementStepResult StepResult = Statement.Step();

        if (StepResult == ESQLitePreparedStatementStepResult::Done)
        {
            break;
        }

        if (StepResult != ESQLitePreparedStatementStepResult::Row)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to query contributor details: %s"), *Database.GetLastError());

            return false;
        }

        FProjectActivityContributorStats Stats;

        if (!Statement.GetColumnValueByIndex(0, Stats.ContributorAlias) || !Statement.GetColumnValueByIndex(1, Stats.SaveCount))
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read contributor details row."));

            return false;
        }

        Results.Add(MoveTemp(Stats));
    }

    OutStats = MoveTemp(Results);

    return true;
}

bool FProjectActivityStorage::GetPackageActivityBucketStats(int32 Limit, TArray<FProjectActivityBucketStats>& OutStats, const FProjectActivityTimeRange& TimeRange)
{
    OutStats.Reset();

    if (!Database.IsValid())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot query activity buckets: database is not open."));

        return false;
    }

    if (Limit < 0)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Activity bucket limit must not be negative."));

        return false;
    }

    const TCHAR* QuerySql =
        TEXT("WITH FilteredActivity AS (")
        TEXT("SELECT PackagePath, ")
        TEXT("TRIM(ContributorAlias) AS Alias, ")
        TEXT("CAST(strftime('%s', TimestampUtc) AS INTEGER) / 300 ")
        TEXT("AS BucketId ")
        TEXT("FROM Activity ")
        TEXT("WHERE ActivityType = ?1 ")
        TEXT("AND (?2 = 0 OR ")
        TEXT("(TimestampUtc >= ?3 AND TimestampUtc <= ?4))")
        TEXT("), BucketStats AS (")
        TEXT("SELECT PackagePath, Alias, BucketId, ")
        TEXT("COUNT(*) AS SaveCount ")
        TEXT("FROM FilteredActivity ")
        TEXT("GROUP BY PackagePath, Alias, BucketId")
        TEXT(") ")
        TEXT("SELECT PackagePath, ")
        TEXT("SUM(SaveCount) AS RawSaveCount, ")
        TEXT("COUNT(*) AS ActivityBucketCount ")
        TEXT("FROM BucketStats ")
        TEXT("GROUP BY PackagePath ")
        TEXT("ORDER BY ActivityBucketCount DESC, ")
        TEXT("RawSaveCount DESC, PackagePath ASC ")
        TEXT("LIMIT ?5;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to prepare activity buckets: %s"), *Database.GetLastError());

        return false;
    }

    const int32 SqlLimit = Limit == 0 ? -1 : Limit;

    const bool bBindingsSucceeded =Statement.SetBindingValueByIndex(1, static_cast<int32>(EProjectActivityType::PackageSaved)) && BindActivityTimeRange(Statement, 2, TimeRange) && Statement.SetBindingValueByIndex(5, SqlLimit);

    if (!bBindingsSucceeded)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to bind activity buckets: %s"), *Database.GetLastError());

        return false;
    }

    TArray<FProjectActivityBucketStats> Results;

    while (true)
    {
        const ESQLitePreparedStatementStepResult StepResult = Statement.Step();

        if (StepResult == ESQLitePreparedStatementStepResult::Done)
        {
            break;
        }

        if (StepResult != ESQLitePreparedStatementStepResult::Row)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to query activity buckets: %s"), *Database.GetLastError());

            return false;
        }

        FProjectActivityBucketStats Stats;

        const bool bReadSucceeded =Statement.GetColumnValueByIndex(0, Stats.PackagePath) && Statement.GetColumnValueByIndex(1, Stats.RawSaveCount) && Statement.GetColumnValueByIndex(2, Stats.ActivityBucketCount);

        if (!bReadSucceeded)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read activity bucket row."));

            return false;
        }

        Results.Add(MoveTemp(Stats));
    }

    OutStats = MoveTemp(Results);

    return true;
}

void FProjectActivityStorage::Close()
{
    if (Database.IsValid() && !Database.Close())
    {
        UE_LOG(LogProjectActivityStorage , Error , TEXT("Failed to close activity database: %s") , *Database.GetLastError());
    }
}

static bool BindActivityTimeRange(FSQLitePreparedStatement& Statement , int32 FirstBindingIndex , const FProjectActivityTimeRange& TimeRange)
{
    if (TimeRange.bEnabled && TimeRange.StartUtc > TimeRange.EndUtc)
    {
        UE_LOG(LogProjectActivityStorage, Error , TEXT("Activity time range start is after its end."));

        return false;
    }

    const FString StartText =
        TimeRange.bEnabled
        ? TimeRange.StartUtc.ToIso8601()
        : FString();

    const FString EndText =
        TimeRange.bEnabled
        ? TimeRange.EndUtc.ToIso8601()
        : FString();

    return Statement.SetBindingValueByIndex(
        FirstBindingIndex,
        static_cast<int32>(TimeRange.bEnabled))
        && Statement.SetBindingValueByIndex(
            FirstBindingIndex + 1,
            StartText)
        && Statement.SetBindingValueByIndex(
            FirstBindingIndex + 2,
            EndText);
}

bool FProjectActivityStorage::SaveSourceControlSnapshot(const FProjectActivitySourceControlSnapshot& Snapshot)
{
    if (!Database.IsValid())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot save source control snapshot: database is not open."));

        return false;
    }

    if (Snapshot.PackagePath.IsEmpty()
        || Snapshot.ProviderName.IsEmpty()
        || Snapshot.FetchedAtUtc.GetTicks() <= 0
        || Snapshot.RetrievedHistoryCount < 0
        || Snapshot.RecentRevisions.Num() > 5
        || Snapshot.RecentRevisions.Num()
            > Snapshot.RetrievedHistoryCount)
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot save source control snapshot: invalid snapshot data."));

        return false;
    }

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

    Root->SetNumberField(TEXT("SchemaVersion"), 1);
    Root->SetBoolField(TEXT("Tracked"), Snapshot.bTracked);
    Root->SetBoolField(TEXT("Modified"), Snapshot.bModified);
    Root->SetBoolField(TEXT("Added"), Snapshot.bAdded);
    Root->SetBoolField(TEXT("Conflicted"), Snapshot.bConflicted);

    Root->SetNumberField(TEXT("RetrievedHistoryCount"), Snapshot.RetrievedHistoryCount);

    TArray<TSharedPtr<FJsonValue>> RevisionValues;

    for (const FProjectActivitySourceControlRevision& Revision : Snapshot.RecentRevisions)
    {
        if (Revision.RevisionDisplayId.IsEmpty() || Revision.RevisionTimeUtc.GetTicks() <= 0)
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Cannot save source control snapshot: invalid revision data."));

            return false;
        }

        TSharedRef<FJsonObject> RevisionObject = MakeShared<FJsonObject>();

        RevisionObject->SetStringField(TEXT("RevisionDisplayId"), Revision.RevisionDisplayId);

        RevisionObject->SetStringField(TEXT("AuthorName"), Revision.AuthorName);

        RevisionObject->SetStringField(TEXT("RevisionTimeUtc"), Revision.RevisionTimeUtc.ToIso8601());

        RevisionObject->SetStringField(TEXT("Description"), Revision.Description);

        RevisionValues.Add(MakeShared<FJsonValueObject>(RevisionObject));
    }

    Root->SetArrayField(TEXT("RecentRevisions"), RevisionValues);

    FString SnapshotJson;

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SnapshotJson);

    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to serialize source control snapshot."));

        return false;
    }

    const TCHAR* SaveSql =
        TEXT("INSERT INTO SourceControlSnapshotCache ")
        TEXT("(PackagePath, ProviderName, RepositoryRoot, ")
        TEXT("BranchName, FetchedAtUtc, SnapshotJson) ")
        TEXT("VALUES (?1, ?2, ?3, ?4, ?5, ?6) ")
        TEXT("ON CONFLICT(PackagePath) DO UPDATE SET ")
        TEXT("ProviderName = excluded.ProviderName, ")
        TEXT("RepositoryRoot = excluded.RepositoryRoot, ")
        TEXT("BranchName = excluded.BranchName, ")
        TEXT("FetchedAtUtc = excluded.FetchedAtUtc, ")
        TEXT("SnapshotJson = excluded.SnapshotJson;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, SaveSql))
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to prepare source control snapshot save: %s"), *Database.GetLastError());

        return false;
    }

    const bool bBindingsSucceeded =
        Statement.SetBindingValueByIndex(1, Snapshot.PackagePath)
        && Statement.SetBindingValueByIndex(2, Snapshot.ProviderName)
        && Statement.SetBindingValueByIndex(3, Snapshot.RepositoryRoot)
        && Statement.SetBindingValueByIndex(4, Snapshot.BranchName)
        && Statement.SetBindingValueByIndex(5, Snapshot.FetchedAtUtc.ToIso8601())
        && Statement.SetBindingValueByIndex(6, SnapshotJson);

    if (!bBindingsSucceeded || !Statement.Execute())
    {
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to save source control snapshot: %s"), *Database.GetLastError());

        return false;
    }

    UE_LOG(
        LogProjectActivityStorage,
        Log,
        TEXT("Source control snapshot cached | Package=%s | StoredRevisions=%d | FetchedAtUtc=%s"),
        *Snapshot.PackagePath,
        Snapshot.RecentRevisions.Num(),
        *Snapshot.FetchedAtUtc.ToIso8601());

    return true;
}

bool FProjectActivityStorage::LoadSourceControlSnapshot(const FString& PackagePath, FProjectActivitySourceControlSnapshot& OutSnapshot, bool& OutFound)
{
    const FString RequestedPackagePath = PackagePath;

    OutSnapshot = FProjectActivitySourceControlSnapshot();
    OutFound = false;

    const auto Fail =
        [&RequestedPackagePath](const FString& Message) -> bool
        {
            UE_LOG(
                LogProjectActivityStorage,
                Error,
                TEXT("Failed to load source control snapshot: %s | RequestedPackage=%s"),
                *Message,
                *RequestedPackagePath);

            return false;
        };

    if (!Database.IsValid())
    {
        return Fail(TEXT("Database is not open."));
    }

    const TCHAR* QuerySql =
        TEXT("SELECT PackagePath, ProviderName, RepositoryRoot, ")
        TEXT("BranchName, FetchedAtUtc, SnapshotJson ")
        TEXT("FROM SourceControlSnapshotCache ")
        TEXT("WHERE (?1 = '' OR PackagePath = ?1) ")
        TEXT("ORDER BY FetchedAtUtc DESC, PackagePath ASC ")
        TEXT("LIMIT 1;");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, QuerySql) || !Statement.SetBindingValueByIndex(1, RequestedPackagePath))
    {
        return Fail(Database.GetLastError());
    }

    const ESQLitePreparedStatementStepResult StepResult = Statement.Step();

    if (StepResult == ESQLitePreparedStatementStepResult::Done)
    {
        // 조회는 성공했지만 저장된 캐시가 없다.
        return true;
    }

    if (StepResult != ESQLitePreparedStatementStepResult::Row)
    {
        return Fail(Database.GetLastError());
    }

    FProjectActivitySourceControlSnapshot Snapshot;

    FString FetchedAtText;
    FString SnapshotJson;

    if (!Statement.GetColumnValueByIndex(0, Snapshot.PackagePath)
        || !Statement.GetColumnValueByIndex(1, Snapshot.ProviderName)
        || !Statement.GetColumnValueByIndex(2, Snapshot.RepositoryRoot)
        || !Statement.GetColumnValueByIndex(3, Snapshot.BranchName)
        || !Statement.GetColumnValueByIndex(4, FetchedAtText)
        || !Statement.GetColumnValueByIndex(5, SnapshotJson))
    {
        return Fail(TEXT("Could not read snapshot columns."));
    }

    if (Snapshot.PackagePath.IsEmpty()
        || Snapshot.ProviderName.IsEmpty()
        || !FDateTime::ParseIso8601(
            *FetchedAtText,
            Snapshot.FetchedAtUtc)
        || Snapshot.FetchedAtUtc.GetTicks() <= 0)
    {
        return Fail(TEXT("Invalid snapshot metadata."));
    }

    TSharedPtr<FJsonObject> Root;

    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SnapshotJson);

    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return Fail(TEXT("Invalid snapshot JSON."));
    }

    int32 SchemaVersion = 0;

    const TArray<TSharedPtr<FJsonValue>>* RevisionValues = nullptr;

    if (!Root->TryGetNumberField(
        TEXT("SchemaVersion"),
        SchemaVersion)
        || SchemaVersion != 1
        || !Root->TryGetBoolField(
            TEXT("Tracked"),
            Snapshot.bTracked)
        || !Root->TryGetBoolField(
            TEXT("Modified"),
            Snapshot.bModified)
        || !Root->TryGetBoolField(
            TEXT("Added"),
            Snapshot.bAdded)
        || !Root->TryGetBoolField(
            TEXT("Conflicted"),
            Snapshot.bConflicted)
        || !Root->TryGetNumberField(
            TEXT("RetrievedHistoryCount"),
            Snapshot.RetrievedHistoryCount)
        || Snapshot.RetrievedHistoryCount < 0
        || !Root->TryGetArrayField(
            TEXT("RecentRevisions"),
            RevisionValues)
        || RevisionValues == nullptr
        || RevisionValues->Num() > 5
        || RevisionValues->Num() > Snapshot.RetrievedHistoryCount)
    {
        return Fail(TEXT("Invalid snapshot fields or schema version."));
    }

    for (const TSharedPtr<FJsonValue>& RevisionValue : *RevisionValues)
    {
        const TSharedPtr<FJsonObject>* RevisionObject = nullptr;

        if (!RevisionValue.IsValid()
            || !RevisionValue->TryGetObject(RevisionObject)
            || RevisionObject == nullptr
            || !RevisionObject->IsValid())
        {
            return Fail(TEXT("Invalid cached revision object."));
        }

        const TSharedPtr<FJsonObject>& Item = *RevisionObject;

        FProjectActivitySourceControlRevision Revision;
        FString RevisionTimeText;

        if (!Item->TryGetStringField(
            TEXT("RevisionDisplayId"),
            Revision.RevisionDisplayId)
            || Revision.RevisionDisplayId.IsEmpty()
            || !Item->TryGetStringField(
                TEXT("AuthorName"),
                Revision.AuthorName)
            || !Item->TryGetStringField(
                TEXT("RevisionTimeUtc"),
                RevisionTimeText)
            || !FDateTime::ParseIso8601(
                *RevisionTimeText,
                Revision.RevisionTimeUtc)
            || Revision.RevisionTimeUtc.GetTicks() <= 0
            || !Item->TryGetStringField(
                TEXT("Description"),
                Revision.Description))
        {
            return Fail(TEXT("Invalid cached revision fields."));
        }

        Snapshot.RecentRevisions.Add(MoveTemp(Revision));
    }

    OutSnapshot = MoveTemp(Snapshot);
    OutFound = true;

    UE_LOG(
        LogProjectActivityStorage,
        Log,
        TEXT("Source control snapshot loaded | Package=%s | StoredRevisions=%d | FetchedAtUtc=%s"),
        *OutSnapshot.PackagePath,
        OutSnapshot.RecentRevisions.Num(),
        *OutSnapshot.FetchedAtUtc.ToIso8601());

    return true;
}