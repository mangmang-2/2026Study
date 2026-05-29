#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IconGeneratorHelper.generated.h"

class UTextureRenderTarget2D;
class UTexture2D;
class USkeletalMesh;
class UStaticMesh;

UENUM(BlueprintType)
enum class EIconMeshType : uint8
{
    Skeletal,
    Static,
};

// 에디터 전용 — EUW_IconGenerator 에서 호출
UCLASS()
class STUDYPROJECT_API UIconGeneratorHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // 폴더 내 메시 에셋 경로 목록 반환 (AssetRegistry 검색)
    // FolderPath 예: "/Game/Meshes/Weapons"
    UFUNCTION(BlueprintCallable, Category = "IconGenerator",
        meta = (DevelopmentOnly))
    static TArray<FString> GetMeshAssetsInFolder(
        const FString& FolderPath,
        EIconMeshType MeshType = EIconMeshType::Skeletal);

    // 경로 문자열에서 에셋 이름만 추출 (T_Icon_ 접두사용)
    // "/Game/Meshes/SK_Sword01" → "SK_Sword01"
    UFUNCTION(BlueprintPure, Category = "IconGenerator")
    static FString GetAssetNameFromPath(const FString& AssetPath);

    // RenderTarget → Texture2D .uasset 저장 후 반환
    // SavePath 예: "/Game/Textures/Icons/"
    UFUNCTION(BlueprintCallable, Category = "IconGenerator",
        meta = (DevelopmentOnly))
    static UTexture2D* CaptureAndSaveIcon(
        UTextureRenderTarget2D* RenderTarget,
        const FString& AssetName,
        const FString& SavePath = TEXT("/Game/Textures/Icons/"));

    // 스켈레탈 메시 에셋 경로로 USkeletalMesh 로드
    UFUNCTION(BlueprintCallable, Category = "IconGenerator",
        meta = (DevelopmentOnly))
    static USkeletalMesh* LoadSkeletalMesh(const FString& AssetPath);

    // 스태틱 메시 에셋 경로로 UStaticMesh 로드
    UFUNCTION(BlueprintCallable, Category = "IconGenerator",
        meta = (DevelopmentOnly))
    static UStaticMesh* LoadStaticMesh(const FString& AssetPath);
};
