#include "ProjectActivityStorage.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "SQLitePreparedStatement.h"
#include "Misc/Timespan.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectActivityStorage, Log, All);

static bool BindActivityTimeRange(FSQLitePreparedStatement& Statement , int32 FirstBindingIndex , const FProjectActivityTimeRange& TimeRange);

FProjectActivityStorage::~FProjectActivityStorage()
{
    Close();
}

bool FProjectActivityStorage::Initialize()
{
    // 이미 열려 있으면 다시 초기화하지 않는다.
    if (Database.IsValid())
    {
        return true;
    }

    const FString StorageDirectory =FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("ProjectActivity")));

    if (!IFileManager::Get().MakeDirectory(*StorageDirectory,true))
    {
        UE_LOG(LogProjectActivityStorage,Error,TEXT("Failed to create storage directory: %s"),*StorageDirectory);

        return false;
    }

    const FString DatabasePath =FPaths::Combine(StorageDirectory,TEXT("activity.db"));

    if (!Database.Open(*DatabasePath,ESQLiteDatabaseOpenMode::ReadWriteCreate))
    {
        UE_LOG(LogProjectActivityStorage,Error,TEXT("Failed to open activity database: %s"),*DatabasePath);

        return false;
    }

    const TCHAR* CreateTableSql =
        TEXT("CREATE TABLE IF NOT EXISTS Activity (")
        TEXT("EventId TEXT PRIMARY KEY NOT NULL,")
        TEXT("PackagePath TEXT NOT NULL,")
        TEXT("TimestampUtc TEXT NOT NULL,")
        TEXT("ActivityType INTEGER NOT NULL")
        TEXT(");");

    if (!Database.Execute(CreateTableSql))
    {
        UE_LOG(LogProjectActivityStorage,Error,TEXT("Failed to create Activity table: %s"),*Database.GetLastError());

        Close();
        return false;
    }

    UE_LOG(LogProjectActivityStorage,Log,TEXT("Activity database ready: %s"),*DatabasePath);

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
        TEXT("(EventId, PackagePath, TimestampUtc, ActivityType) ")
        TEXT("VALUES (?1, ?2, ?3, ?4);");

    FSQLitePreparedStatement Statement;

    if (!Statement.Create(Database, InsertSql))
    {
        UE_LOG( LogProjectActivityStorage, Error, TEXT("Failed to prepare activity insert: %s"), *Database.GetLastError());

        return false;
    }

    const FString EventIdText = Record.EventId.ToString(EGuidFormats::DigitsWithHyphens);

    const FString TimestampText = Record.TimestampUtc.ToIso8601();

    const bool bBindingsSucceeded = Statement.SetBindingValueByIndex(1, EventIdText)
        && Statement.SetBindingValueByIndex(2, Record.PackagePath)
        && Statement.SetBindingValueByIndex(3, TimestampText)
        && Statement.SetBindingValueByIndex(
            4, static_cast<int32>(Record.ActivityType));

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

    // UTC 오늘의 00:00에서 이전 날짜로 이동한다.
    const FDateTime FirstDayUtc =NowUtc.GetDate() - FTimespan::FromDays(static_cast<double>(NumDays - 1));

    const TCHAR* QuerySql =
        TEXT("SELECT substr(TimestampUtc, 1, 10) AS DayUtc, ")
        TEXT("COUNT(*) AS SaveCount ")
        TEXT("FROM Activity ")
        TEXT("WHERE ActivityType = ?1 ")
        TEXT("AND TimestampUtc >= ?2 ")
        TEXT("AND TimestampUtc <= ?3 ")
        TEXT("GROUP BY substr(TimestampUtc, 1, 10) ")
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
        UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to bind daily stats: %s"),*Database.GetLastError());

        return false;
    }

    TMap<FString, int64> CountsByDay;

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
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to query daily stats: %s"), *Database.GetLastError());

            return false;
        }

        FString DateUtc;
        int64 SaveCount = 0;

        if (!Statement.GetColumnValueByIndex(0, DateUtc) || !Statement.GetColumnValueByIndex(1, SaveCount))
        {
            UE_LOG(LogProjectActivityStorage, Error, TEXT("Failed to read daily stats row."));

            return false;
        }

        CountsByDay.Add(DateUtc, SaveCount);
    }

    // DB에 기록이 없는 날짜도 포함해서 결과를 만든다.
    TArray<FProjectActivityDailyStats> Results;
    Results.Reserve(NumDays);

    for (int32 DayIndex = 0; DayIndex < NumDays; ++DayIndex)
    {
        const FDateTime DayUtc = FirstDayUtc + FTimespan::FromDays(static_cast<double>(DayIndex));

        FProjectActivityDailyStats Stats;
        Stats.DateUtc = DayUtc.ToString(TEXT("%Y-%m-%d"));

        const int64* FoundCount = CountsByDay.Find(Stats.DateUtc);

        Stats.SaveCount = FoundCount != nullptr ? *FoundCount : 0;

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