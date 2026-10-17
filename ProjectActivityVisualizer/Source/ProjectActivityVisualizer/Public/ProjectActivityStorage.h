#pragma once

#pragma once

#include "CoreMinimal.h"
#include "SQLiteDatabase.h"
#include "ProjectActivityTypes.h"

class FProjectActivityStorage
{
public:

    ~FProjectActivityStorage();

    // DB를 열고 필요한 테이블을 준비한다.
    bool Initialize();

    // 활동 기록 하나를 DB에 저장한다.
    bool InsertRecord(const FProjectActivityRecord& Record);

    bool GetRecordCount(int64& OutCount, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    bool GetMostSavedPackages(int32 Limit, TArray<FProjectActivityPackageStats>& OutStats, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    // 열린 DB 연결을 닫는다.
    void Close();

    bool GetPackageSaveCounts(TArray<FProjectActivityPackageStats>& OutStats, const FProjectActivityTimeRange& TimeRange =
        FProjectActivityTimeRange());

    bool GetCategorySaveCounts(TArray<FProjectActivityCategoryStats>& OutStats, const FProjectActivityTimeRange& TimeRange =
        FProjectActivityTimeRange());

    // UTC 기준 오늘을 포함한 NumDays일의 저장 횟수를 조회한다.
    bool GetRecentDailySaveCounts(int32 NumDays, TArray<FProjectActivityDailyStats>& OutStats, const FDateTime& NowUtc);

private:
    FSQLiteDatabase Database;
};