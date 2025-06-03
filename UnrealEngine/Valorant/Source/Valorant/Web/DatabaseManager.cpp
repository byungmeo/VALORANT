// Fill out your copyright notice in the Description page of Project Settings.


#include "DatabaseManager.h"

#include "HttpManager.h"
#include "JsonObjectConverter.h"
#include "Interfaces/IHttpResponse.h"

UDatabaseManager* UDatabaseManager::Singleton = nullptr;
FString UDatabaseManager::DatabaseUrl = "";

/* static */ UDatabaseManager* UDatabaseManager::GetInstance()
{
	if (!Singleton)
	{
		Singleton = NewObject<UDatabaseManager>();
		Singleton->AddToRoot();
		GConfig->GetString(TEXT("APIServerUrl"), TEXT("Url"), DatabaseUrl, *FConfigCacheIni::NormalizeConfigIniPath(FPaths::ProjectConfigDir() / TEXT("Secret.ini")));
	}
	return Singleton;
}

void UDatabaseManager::GetPlayer(const FString& PlayerId, const FString& Platform, const FOnGetPlayerCompleted& Callback)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("player_id"), PlayerId);
	Json->SetStringField(TEXT("platform"), Platform);
	FString q;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&q);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);
	UE_LOG(LogTemp, Warning, TEXT("q: %s"), *q);

	FHttpQuery Query;
	Query.AddQuery("q", q);
	const FString& Url = Query.AppendToURL(DatabaseUrl + TEXT("player/"));
	UE_LOG(LogTemp, Warning, TEXT("Url: %s"), *Url);
	UHttpManager::GetInstance()->SendRequest(Url, TEXT("GET"), TEXT(""),
		[=](FHttpResponsePtr Response, bool bSuccess)
		{
			FPlayerDTO PlayerDto;
			
			if (!bSuccess || !Response.IsValid())
			{
				Callback.Broadcast(false, PlayerDto);
				return;
			}

			TSharedPtr<FJsonObject> RootJson;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (FJsonSerializer::Deserialize(Reader, RootJson) && RootJson.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* Items;
				if (RootJson->TryGetArrayField(TEXT("items"), Items) && Items->Num() > 0)
				{
					TSharedPtr<FJsonObject> PlayerObj = (*Items)[0]->AsObject();
					if (FJsonObjectConverter::JsonObjectToUStruct(PlayerObj.ToSharedRef(), &PlayerDto))
					{
						Callback.Broadcast(true, PlayerDto);
						return;
					}
				}
			}
			UE_LOG(LogTemp, Error, TEXT("파싱 실패 또는 items[0] 없음"));
			Callback.Broadcast(false, PlayerDto);
		}
	);
}

void UDatabaseManager::PostPlayer(const FString& PlayerId, const FString& Platform)
{
	FPlayerDTO PlayerDto;
	PlayerDto.player_id = PlayerId;
	PlayerDto.platform = Platform;
	FString JsonString;
	FJsonObjectConverter::UStructToJsonObjectString(PlayerDto, JsonString);
	UHttpManager::GetInstance()->SendRequest(DatabaseUrl + TEXT("player/"), TEXT("POST"), JsonString);
}

void UDatabaseManager::PutPlayer(const FPlayerDTO& PlayerDto)
{
	FString JsonString;
	FJsonObjectConverter::UStructToJsonObjectString(PlayerDto, JsonString);
	UHttpManager::GetInstance()->SendRequest(DatabaseUrl + TEXT("player/"), TEXT("PUT"), JsonString);
}

void UDatabaseManager::GetMatch(const int MatchId, const FOnGetMatchCompleted& Callback)
{
	UHttpManager::GetInstance()->SendRequest(DatabaseUrl + FString::Printf(TEXT("match/%d"), MatchId), TEXT("GET"), "",
	[=](const FHttpResponsePtr& Response, const bool bSuccess)
	{
		if (!bSuccess || !Response.IsValid()) {
			Callback.Broadcast(false, FMatchDTO());
			return;
		}

		TSharedPtr<FJsonObject> ResponseJson;
		const FString ResponseBody = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		if (FJsonSerializer::Deserialize(Reader, ResponseJson) && ResponseJson.IsValid())
		{
			FMatchDTO MatchDto;
			FJsonObjectConverter::JsonObjectToUStruct(ResponseJson.ToSharedRef(), FMatchDTO::StaticStruct(), &MatchDto);
			Callback.Broadcast(bSuccess, MatchDto);
		}
		else
		{
			Callback.Broadcast(false, FMatchDTO());
		}
	});
}

void UDatabaseManager::PostMatch(const FOnPostMatchCompleted& Callback)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetNumberField("map_id", 0);
	JsonObject->SetNumberField("blue_score", 0);
	JsonObject->SetNumberField("red_score", 0);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject, Writer);
	
	UHttpManager::GetInstance()->SendRequest(DatabaseUrl + TEXT("match/"), TEXT("POST"), JsonString,
	[=](const FHttpResponsePtr& Response, const bool bSuccess)
	{
		if (!bSuccess || !Response.IsValid()) {
			Callback.Broadcast(false, FMatchDTO());
			return;
		}

		TSharedPtr<FJsonObject> ResponseJson;
		const FString ResponseBody = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		if (FJsonSerializer::Deserialize(Reader, ResponseJson) && ResponseJson.IsValid())
		{
			FMatchDTO MatchDto;
			FJsonObjectConverter::JsonObjectToUStruct(ResponseJson.ToSharedRef(), FMatchDTO::StaticStruct(), &MatchDto);
			Callback.Broadcast(bSuccess, MatchDto);
		}
		else
		{
			Callback.Broadcast(false, FMatchDTO());
		}
	});
}

void UDatabaseManager::PutMatch(const int MatchId, const FMatchDTO& MatchDto)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	JsonObject->SetNumberField("map_id", MatchDto.map_id);
	JsonObject->SetNumberField("blue_score", MatchDto.blue_score);
	JsonObject->SetNumberField("red_score", MatchDto.red_score);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject, Writer);
	
	UHttpManager::GetInstance()->SendRequest(DatabaseUrl + FString::Printf(TEXT("match/%d"), MatchId), TEXT("PUT"), JsonString);
}

void UDatabaseManager::GetPlayerMatch(const FString& PlayerId, const int MatchId, const FOnGetPlayerMatchCompleted& Callback)
{
	TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("player_id"), PlayerId);
	Json->SetNumberField(TEXT("match_id"), MatchId);
	FString q;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&q);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);
	UE_LOG(LogTemp, Warning, TEXT("q: %s"), *q);

	FHttpQuery Query;
	Query.AddQuery("q", q);
	const FString& Url = Query.AppendToURL(DatabaseUrl + TEXT("player_match/"));
	UE_LOG(LogTemp, Warning, TEXT("Url: %s"), *Url);
	
	UHttpManager::GetInstance()->SendRequest(Url, TEXT("GET"), TEXT(""),
	[=](const FHttpResponsePtr& Response, const bool bSuccess)
	{
		if (!bSuccess || !Response.IsValid()) {
			Callback.Broadcast(false, FPlayerMatchDTO());
			return;
		}

		TSharedPtr<FJsonObject> ResponseJson;
		const FString ResponseBody = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
		UE_LOG(LogTemp, Warning, TEXT("ResponseBody: %s"), *ResponseBody);
		if (FJsonSerializer::Deserialize(Reader, ResponseJson) && ResponseJson.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* Items;
			if (ResponseJson->TryGetArrayField(TEXT("items"), Items) && Items && Items->Num() > 0)
			{
				const TSharedPtr<FJsonObject> ItemObj = (*Items)[0]->AsObject();
				if (ItemObj.IsValid())
				{
					FPlayerMatchDTO PlayerMatchDto;
					if (FJsonObjectConverter::JsonObjectToUStruct(ItemObj.ToSharedRef(), FPlayerMatchDTO::StaticStruct(), &PlayerMatchDto))
					{
						Callback.Broadcast(bSuccess, PlayerMatchDto);
						return;
					}
				}
			}
			Callback.Broadcast(false, FPlayerMatchDTO());
		}
	});
}

void UDatabaseManager::PostPlayerMatch(const FPlayerMatchDTO& PlayerMatchDto)
{
	FString JsonString;
	FJsonObjectConverter::UStructToJsonObjectString(PlayerMatchDto, JsonString);
	UHttpManager::GetInstance()->SendRequest(DatabaseUrl + TEXT("player_match/"), TEXT("POST"), JsonString);
}