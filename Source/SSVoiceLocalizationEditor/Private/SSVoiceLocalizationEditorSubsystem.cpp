/**
* Copyright (C) 2020-2025 Schartier Isaac
*
* Official Documentation: https://www.somndus-studio.com
*/


#include "SSVoiceLocalizationEditorSubsystem.h"

#include "SSVoiceLocalizationSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/SSVoiceLocalizationEditorSettings.h"

#define LOCTEXT_NAMESPACE "SSLocalizedVoiceSoundEditor"

void USSVoiceLocalizationEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Try to load the active strategy on subsystem startup
	RefreshStrategy();
}

void USSVoiceLocalizationEditorSubsystem::Deinitialize()
{
	CachedStrategy = nullptr;

	Super::Deinitialize();
}

FAssetRegistryModule& USSVoiceLocalizationEditorSubsystem::GetAssetRegistryModule()
{
	return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		"AssetRegistry");
}

void USSVoiceLocalizationEditorSubsystem::ChangeActiveProfileFromName(FString ProfileName)
{
	auto* AutofillProfile = GetProfileFromName(ProfileName);

	auto* EditorSettings = USSVoiceLocalizationEditorSettings::GetMutableSetting();
	EditorSettings->ActiveVoiceProfileName = AutofillProfile->ProfileName;

	OnVoiceProfileNameChange();
}

void USSVoiceLocalizationEditorSubsystem::OnVoiceProfileNameChange()
{
	RefreshStrategy();
}

USSVoiceAutofillStrategy* USSVoiceLocalizationEditorSubsystem::GetActiveStrategy()
{
	// Already cached and valid
	if (IsValid(CachedStrategy))
	{
		return CachedStrategy;
	}

	return RefreshStrategy();
}

const FSSVoiceAutofillProfile* USSVoiceLocalizationEditorSubsystem::GetActiveProfile() const
{
	const auto* EditorSettings = USSVoiceLocalizationEditorSettings::GetSetting();
	if (!EditorSettings || EditorSettings->AutofillProfiles.Num() == 0)
	{
		return &EditorSettings->FallbackProfile;
	}

	for (const auto& Profile : EditorSettings->AutofillProfiles)
	{
		if (Profile.ProfileName == EditorSettings->ActiveVoiceProfileName)
		{
			return &Profile;
		}
	}

	// If not found (Default internal profile)
	return &EditorSettings->FallbackProfile;
}

const FSSVoiceAutofillProfile* USSVoiceLocalizationEditorSubsystem::GetProfileFromName(FString ProfileName) const
{
	const auto* EditorSettings = USSVoiceLocalizationEditorSettings::GetSetting();
	if (!EditorSettings || EditorSettings->AutofillProfiles.Num() == 0)
	{
		return &EditorSettings->FallbackProfile;
	}

	for (const auto& Profile : EditorSettings->AutofillProfiles)
	{
		if (Profile.ProfileName == ProfileName)
		{
			return &Profile;
		}
	}

	// If not found (Default internal profile)
	return &EditorSettings->FallbackProfile;
}

bool USSVoiceLocalizationEditorSubsystem::IsReady() const
{
	return CachedStrategy != nullptr;
}

TArray<FAssetData> USSVoiceLocalizationEditorSubsystem::GetAllSoundBaseAssets(bool bRecursivePaths)
{
	FAssetRegistryModule& AssetRegistry = GetAssetRegistryModule();
	AssetRegistry.Get().SearchAllAssets(true);

	FARFilter Filter;
	Filter.ClassPaths.Add(USoundBase::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = bRecursivePaths;
	Filter.PackagePaths.Add(FName("/Game"));

	TArray<FAssetData> FoundAssets;
	AssetRegistry.Get().GetAssets(Filter, FoundAssets);

	return FoundAssets;
}

TArray<FAssetData> USSVoiceLocalizationEditorSubsystem::GetAllLocalizeVoiceSoundAssets()
{
	// 1. Prepare registry
	FAssetRegistryModule& AssetRegistry = GetAssetRegistryModule();

	FARFilter Filter;
	Filter.ClassPaths.Add(USSLocalizedVoiceSound::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName("/Game"));

	TArray<FAssetData> FoundAssets;
	AssetRegistry.Get().GetAssets(Filter, FoundAssets);

	return FoundAssets;
}

void USSVoiceLocalizationEditorSubsystem::GetAssetsFromVoiceActor(TArray<FAssetData>& Assets, FString VoiceActorName,
                                                                  const bool bShowSlowTask)
{
	FAssetRegistryModule& AssetRegistry = GetAssetRegistryModule();

	if (AssetRegistry.Get().IsLoadingAssets()) return;

	TArray<FAssetData> AssetsAll;
	TSet<FAssetData> VoiceActorAssets;
	/*
	FScopedSlowTask SlowTask{
		static_cast<float>(AssetsAll.Num()),
		NSLOCTEXT("SSVoice", "SearchVoiceActorSoundAssets", "Searching used voice actor sound assets..."),
		bShowSlowTask && GIsEditor && !IsRunningCommandlet()
	};
	SlowTask.MakeDialog(false, false);
	*/

	// 1. Retrieve assets
	Assets = GetAllLocalizeVoiceSoundAssets();

	for (const auto& AssetData : Assets)
	{
		const FString Name = AssetData.AssetName.ToString();

		if (!Name.Contains(VoiceActorName))
			continue;


		VoiceActorAssets.Add(AssetData);
	}

	// SlowTask.EnterProgressFrame(1.0f, NSLOCTEXT("SSVoice", "Complete", "Complete"));

	Assets = VoiceActorAssets.Array();
}

void USSVoiceLocalizationEditorSubsystem::GetAssetsWithCulture(TArray<FAssetData>& Assets, const bool bCompleteCulture,
                                                               const bool bShowSlowTask)
{
	FAssetRegistryModule& AssetRegistry = GetAssetRegistryModule();

	if (AssetRegistry.Get().IsLoadingAssets()) return;

	const USSVoiceLocalizationSettings* Settings = USSVoiceLocalizationSettings::GetSetting();
	const TSet<FString> AllCultures = Settings ? Settings->SupportedVoiceCultures : TSet<FString>();
	int32 AllCultureCount = AllCultures.Num();

	// Avoid array edition inf iteration
	TArray<FAssetData> AllAssets = Assets;

	for (const auto& AssetData : AllAssets)
	{
		const FAssetTagValueRef CultureTag = AssetData.TagsAndValues.FindTag("VoiceCultures");
		TSet<FString> PresentCultures;

		if (CultureTag.IsSet())
		{
			const FString CultureString = CultureTag.GetValue();

			TArray<FString> Cultures;
			CultureString.ParseIntoArray(Cultures, TEXT(","));

			for (const FString& Culture : Cultures)
			{
				const FString Normalized = Culture.ToLower();
				PresentCultures.Add(Normalized);
			}
		}

		// bCompleteCulture => Include else Exclude
		if (bCompleteCulture)
		{
			if (PresentCultures.Num() != AllCultureCount)
			{
				Assets.Remove(AssetData);
			}
		}
		else
		{
			if (PresentCultures.Num() == AllCultureCount)
			{
				Assets.Remove(AssetData);
			}
		}
	}
}

void USSVoiceLocalizationEditorSubsystem::GetAssetsWithMissingCulture(TArray<FAssetData>& Assets,
                                                                      const bool bShowSlowTask)
{
	GetAssetsWithCulture(Assets, false, bShowSlowTask);
}

void USSVoiceLocalizationEditorSubsystem::GetAssetsWithCompleteCulture(TArray<FAssetData>& Assets,
                                                                       const bool bShowSlowTask)
{
	GetAssetsWithCulture(Assets, true, bShowSlowTask);
}

FContentBrowserModule& USSVoiceLocalizationEditorSubsystem::GetVoiceContentBrowser()
{
	return FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
}

USSVoiceAutofillStrategy* USSVoiceLocalizationEditorSubsystem::RefreshStrategy()
{
	const FSSVoiceAutofillProfile* ActiveProfile = GetActiveProfile();
	if (!ActiveProfile || !ActiveProfile->StrategyClass.IsValid())
	{
		return nullptr;
	}

	// Instantiate the strategy
	UClass* StrategyClass = ActiveProfile->StrategyClass.LoadSynchronous();
	if (!StrategyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SSVoice] Failed to load autofill strategy class"));
		return nullptr;
	}

	CachedStrategy = NewObject<USSVoiceAutofillStrategy>(GetTransientPackage(), StrategyClass);
	return CachedStrategy;
}

#undef LOCTEXT_NAMESPACE
