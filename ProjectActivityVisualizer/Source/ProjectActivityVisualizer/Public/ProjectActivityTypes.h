#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"

// 활동 이벤트의 종류
enum class EProjectActivityType : uint8
{
    PackageSaved = 0
};

// 저장 이벤트 하나에 대한 기록
struct FProjectActivityRecord
{
    // 기록 생성 시 새로운 이벤트 ID를 만든다.
    FGuid EventId = FGuid::NewGuid();

    // 저장된 패키지 경로
    FString PackagePath;

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

// 카테고리별 저장 횟수 집계 결과
struct FProjectActivityCategoryStats
{
    FName CategoryName = NAME_None;

    int64 SaveCount = 0;
};

// UTC 날짜별 저장 횟수
struct FProjectActivityDailyStats
{
    // YYYY-MM-DD 형식
    FString DateUtc;

    int64 SaveCount = 0;
};