#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"

// 활동 이벤트의 종류
enum class EProjectActivityType : uint8
{
    PackageSaved = 0
};

// 대시보드에서 선택할 활동 조회 기간
enum class EProjectActivityTimePreset : uint8
{
    AllTime,
    Last24Hours,
    Last7Days,
    Last30Days
};

// 저장 이벤트 하나에 대한 기록
struct FProjectActivityRecord
{
    // 기록 생성 시 새로운 이벤트 ID를 만든다.
    FGuid EventId = FGuid::NewGuid();

    // 저장된 패키지 경로
    FString PackagePath;

    // 이 활동이 발생한 시점의 Contributor 별칭
    FString ContributorAlias;

    // 기록 생성 시 UTC 시각을 담는다.
    FDateTime TimestampUtc = FDateTime::UtcNow();

    // 현재 수집하는 이벤트는 패키지 저장이다.
    EProjectActivityType ActivityType = EProjectActivityType::PackageSaved;
};

// 패키지 하나의 저장 횟수 집계 결과
struct FProjectActivityPackageStats
{
    FString PackagePath;


    int64 SaveCount = 0;
};

// 활동 기록을 조회할 UTC 기간
struct FProjectActivityTimeRange
{
    // false이면 전체 기간을 조회한다.
    bool bEnabled = false;

    FDateTime StartUtc = FDateTime(0);

    FDateTime EndUtc = FDateTime(0);
};

// 카테고리별 원본 저장 횟수와 활동 구간 수
struct FProjectActivityCategoryStats
{
    FName CategoryName = NAME_None;

    // 원본 저장 이벤트 수
    int64 SaveCount = 0;

    // 소속 패키지들의 활동 구간 수 합계
    int64 ActivityBucketCount = 0;
};

// UTC 날짜별 저장 횟수
// UTC 날짜별 원본 저장 횟수와 활동 구간 수
struct FProjectActivityDailyStats
{
    // YYYY-MM-DD 형식
    FString DateUtc;

    int64 SaveCount = 0;

    int64 ActivityBucketCount = 0;
};

// 여러 Contributor 별칭으로 저장된 패키지의 집계 결과
struct FProjectActivityMultiContributorStats
{
    FString PackagePath;

    int64 SaveCount = 0;

    int64 ContributorCount = 0;
};

// 패키지 하나에서 특정 별칭으로 저장한 횟수
struct FProjectActivityContributorStats
{
    FString ContributorAlias;

    int64 SaveCount = 0;
};

// 원본 저장 횟수와 5분 단위 활동 구간 수
struct FProjectActivityBucketStats
{
    FString PackagePath;

    int64 RawSaveCount = 0;

    int64 ActivityBucketCount = 0;
};

// 소스 컨트롤에서 조회한 파일 이력 한 건
struct FProjectActivitySourceControlRevision
{
    // 화면 표시용 리비전 ID.
    // Git Provider에서는 짧은 커밋 ID일 수 있다.
    FString RevisionDisplayId;

    // Git 등 소스 컨트롤에 기록된 작성자 이름
    FString AuthorName;

    // Provider가 반환한 이력 시각
    FDateTime RevisionTimeUtc = FDateTime(0);

    FString Description;
};

// 패키지 하나의 마지막 소스 컨트롤 조회 결과
struct FProjectActivitySourceControlSnapshot
{
    FString PackagePath;

    FString ProviderName;

    FString RepositoryRoot;

    FString BranchName;

    // 이 결과를 조회한 시각
    FDateTime FetchedAtUtc = FDateTime(0);

    bool bTracked = false;

    bool bModified = false;

    bool bAdded = false;

    bool bConflicted = false;

    // Provider가 반환한 전체 이력 개수
    int32 RetrievedHistoryCount = 0;

    // 실제로 캐시에 보관할 최근 이력
    TArray<FProjectActivitySourceControlRevision> RecentRevisions;

};

// Hotspot 검토에 사용할 패키지별 근거 데이터
struct FProjectActivityHotspotEvidence
{
    FString PackagePath;

    int64 RawSaveCount = 0;

    int64 ActivityBucketCount = 0;

    // -1이면 조회 결과를 확인하지 못한 상태다.
    int64 NamedAliasCount = -1;

    int64 UnspecifiedSaveCount = -1;

    int32 GameDependencyCount = -1;

    int32 GameReferencerCount = -1;
};

// 선정 근거와 해당 조건을 함께 보관한다.
struct FProjectActivityHotspotCandidate
{
    FProjectActivityHotspotEvidence Evidence;

    bool bMultipleAliasActivity = false;

    bool bReferencedActivity = false;
};