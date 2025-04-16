// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Settings/SSVoiceAutofillStrategy.h"
#include "SSVoiceLocalizationTypes.generated.h"

class USSVoiceAutofillStrategy;

USTRUCT(BlueprintType)
struct FSSVoiceAutofillProfile
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Autofill Profile")
	FString ProfileName;
	
	UPROPERTY(EditAnywhere, Category = "Autofill Profile")
	TSoftClassPtr<USSVoiceAutofillStrategy> StrategyClass;
};

/**
 * One culture scan result
 */
USTRUCT()
struct FSSVoiceCultureReportEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FString Culture;

	UPROPERTY()
	int32 TotalAssets = 0;

	UPROPERTY()
	int32 AssetsWithCulture = 0;

	/** Returns 0.0 - 1.0 coverage */
	float GetCoveragePercent() const
	{
		return TotalAssets > 0 ? float(AssetsWithCulture) / float(TotalAssets) : 0.0f;
	}
};

/**
 * Global voice localization status across cultures
 */
USTRUCT()
struct FSSVoiceCultureReport
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSSVoiceCultureReportEntry> Entries;

	/** Timestamp of generation */
	UPROPERTY()
	FDateTime GeneratedAt;
};