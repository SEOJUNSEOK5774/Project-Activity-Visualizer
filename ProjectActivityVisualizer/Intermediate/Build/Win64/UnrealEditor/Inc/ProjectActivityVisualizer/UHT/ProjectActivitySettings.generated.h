// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "ProjectActivitySettings.h"

#ifdef PROJECTACTIVITYVISUALIZER_ProjectActivitySettings_generated_h
#error "ProjectActivitySettings.generated.h already included, missing '#pragma once' in ProjectActivitySettings.h"
#endif
#define PROJECTACTIVITYVISUALIZER_ProjectActivitySettings_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin ScriptStruct FProjectActivityCategoryRule **************************************
struct Z_Construct_UScriptStruct_FProjectActivityCategoryRule_Statics;
PROJECTACTIVITYVISUALIZER_API UScriptStruct* Z_Construct_UScriptStruct_FProjectActivityCategoryRule(ETypeConstructPhase);

#define FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h_11_GENERATED_BODY \
	friend struct ::Z_Construct_UScriptStruct_FProjectActivityCategoryRule_Statics; \
	UE_NODEBUG static UScriptStruct* StaticStruct() { return Z_Construct_UScriptStruct_FProjectActivityCategoryRule(ETypeConstructPhase::Inner); }


struct FProjectActivityCategoryRule;
// ********** End ScriptStruct FProjectActivityCategoryRule ****************************************

// ********** Begin Class UProjectActivitySettings *************************************************
struct Z_Construct_UClass_UProjectActivitySettings_Statics;
PROJECTACTIVITYVISUALIZER_API UClass* Z_Construct_UClass_UProjectActivitySettings(ETypeConstructPhase);

#define FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h_25_INCLASS_NO_PURE_DECLS \
private: \
	friend struct ::Z_Construct_UClass_UProjectActivitySettings_Statics; \
	friend PROJECTACTIVITYVISUALIZER_API UClass* ::Z_Construct_UClass_UProjectActivitySettings(ETypeConstructPhase); \
public: \
	DECLARE_CLASS2(UProjectActivitySettings, UDeveloperSettings, COMPILED_IN_FLAGS(0 | CLASS_DefaultConfig | CLASS_Config), CASTCLASS_None, TEXT("/Script/ProjectActivityVisualizer"), Z_Construct_UClass_UProjectActivitySettings) \
	DECLARE_SERIALIZER(UProjectActivitySettings) \
	static constexpr const TCHAR* StaticConfigName() {return TEXT("Editor");} \



#define FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h_25_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UProjectActivitySettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	UProjectActivitySettings(UProjectActivitySettings&&) = delete; \
	UProjectActivitySettings(const UProjectActivitySettings&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UProjectActivitySettings); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UProjectActivitySettings); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UProjectActivitySettings) \
	NO_API virtual ~UProjectActivitySettings();


#define FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h_21_PROLOG
#define FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h_25_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h_25_INCLASS_NO_PURE_DECLS \
	FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h_25_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class UProjectActivitySettings;

// ********** End Class UProjectActivitySettings ***************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_chong_Documents_Unreal_Projects_ActivityVisualizer_Plugins_ProjectActivityVisualizer_Source_ProjectActivityVisualizer_Public_ProjectActivitySettings_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
