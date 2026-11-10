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

    // 전체 활동 기록을 JSON 파일로 내보낸다.
    bool ExportAllRecordsToJson( const FString& FilePath, int64& OutRecordCount);

    bool GetRecordCount(int64& OutCount, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    bool GetMostSavedPackages(int32 Limit, TArray<FProjectActivityPackageStats>& OutStats, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    // 열린 DB 연결을 닫는다.
    void Close();

    bool GetPackageSaveCounts(TArray<FProjectActivityPackageStats>& OutStats, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    bool GetCategorySaveCounts(TArray<FProjectActivityCategoryStats>& OutStats, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    // UTC 기준 오늘을 포함한 NumDays일의 저장 횟수를 조회한다.
    bool GetRecentDailySaveCounts(int32 NumDays, TArray<FProjectActivityDailyStats>& OutStats, const FDateTime& NowUtc);

    // JSON 기록을 가져온다. 기존 EventId는 건너뛴다.
    bool ImportRecordsFromJson(const FString& FilePath, int64& OutImportedCount, int64& OutSkippedCount);

    // 선택한 기간에 서로 다른 별칭이 2개 이상 기록된 패키지를 조회한다.
    bool GetMultiContributorPackages(int32 Limit, TArray<FProjectActivityMultiContributorStats>& OutStats, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    // 패키지 하나의 별칭별 저장 횟수를 조회한다.
    bool GetPackageContributorStats(const FString& PackagePath, TArray<FProjectActivityContributorStats>& OutStats, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    // 패키지별 원본 저장 횟수와 활동 구간 수를 조회한다.
    bool GetPackageActivityBucketStats(int32 Limit, TArray<FProjectActivityBucketStats>& OutStats, const FProjectActivityTimeRange& TimeRange = FProjectActivityTimeRange());

    // 패키지의 마지막 소스 컨트롤 조회 결과를 저장하거나 갱신한다.
    bool SaveSourceControlSnapshot(const FProjectActivitySourceControlSnapshot& Snapshot);

    // PackagePath가 비어 있으면 가장 최근 캐시를 조회한다.
    // 반환값은 조회 성공 여부이며, 캐시 존재 여부는 OutFound로 구분한다.
    bool LoadSourceControlSnapshot(const FString& PackagePath, FProjectActivitySourceControlSnapshot& OutSnapshot, bool& OutFound);

private:
    // 기존 DB의 기록을 보존하면서 별칭 컬럼을 준비한다.
    bool EnsureContributorAliasColumn();

    // 마지막 소스 컨트롤 조회 결과를 보관할 테이블을 준비한다.
    bool EnsureSourceControlSnapshotCacheTable();

    FSQLiteDatabase Database;
};