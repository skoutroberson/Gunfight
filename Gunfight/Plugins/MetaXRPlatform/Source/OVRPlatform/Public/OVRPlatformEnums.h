/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * Licensed under the Oculus SDK License Agreement (the "License");
 * you may not use the Oculus SDK except in compliance with the License,
 * which is provided at the time of installation or download, or which
 * otherwise accompanies this software in either electronic or hard copy form.
 *
 * You may obtain a copy of the License at
 *
 * https://developer.oculus.com/licenses/oculussdk/
 *
 * Unless required by applicable law or agreed to in writing, the Oculus SDK
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// This file was @generated with LibOVRPlatform/codegen/main. Do not modify it!

#pragma once

#include "CoreMinimal.h"
#include "OVR_Platform.h"

#include "OVR_AbuseReportType.h"
#include "OVR_AccountAgeCategory.h"
#include "OVR_AchievementType.h"
#include "OVR_AppAgeCategory.h"
#include "OVR_AppInstallResult.h"
#include "OVR_AppStatus.h"
#include "OVR_ChallengeCreationType.h"
#include "OVR_ChallengeViewerFilter.h"
#include "OVR_ChallengeVisibility.h"
#include "OVR_KeyValuePairType.h"
#include "OVR_LaunchResult.h"
#include "OVR_LaunchType.h"
#include "OVR_LeaderboardFilterType.h"
#include "OVR_LeaderboardStartAt.h"
#include "OVR_LivestreamingStartStatus.h"
#include "OVR_LogEventName.h"
#include "OVR_LogEventParameter.h"
#include "OVR_MediaContentType.h"
#include "OVR_MultiplayerErrorErrorKey.h"
#include "OVR_NetSyncConnectionStatus.h"
#include "OVR_NetSyncDisconnectReason.h"
#include "OVR_NetSyncVoipMicSource.h"
#include "OVR_NetSyncVoipStreamMode.h"
#include "OVR_PartyUpdateAction.h"
#include "OVR_PermissionGrantStatus.h"
#include "OVR_PlatformInitializeResult.h"
#include "OVR_ReportRequestResponse.h"
#include "OVR_RichPresenceExtraContext.h"
#include "OVR_SdkAccountType.h"
#include "OVR_ServiceProvider.h"
#include "OVR_ShareMediaStatus.h"
#include "OVR_SystemVoipStatus.h"
#include "OVR_TimeWindow.h"
#include "OVR_UserOrdering.h"
#include "OVR_UserPresenceStatus.h"
#include "OVR_VoipBitrate.h"
#include "OVR_VoipDtxState.h"
#include "OVR_VoipMuteState.h"
#include "OVR_VoipSampleRate.h"

#include "OVRPlatformEnums.generated.h"


/** AbuseReportType enumeration. */
UENUM(BlueprintType)
enum class EOvrAbuseReportType : uint8
{
    Unknown,
    /** A report for something besides a user, like a world. */
    Object,
    /** A report for a user's behavior or profile. */
    User,
};

ovrAbuseReportType ConvertAbuseReportType(EOvrAbuseReportType Value);
EOvrAbuseReportType ConvertAbuseReportType(ovrAbuseReportType Value);

/** Age category in Meta account. The values are used in UserAgeCategory_Get() API. */
UENUM(BlueprintType)
enum class EOvrAccountAgeCategory : uint8
{
    Unknown,
    /** Child age group for users between the ages of 10-12 (age may vary by region) */
    Ch,
    /** Teenage age group for users between the ages of 13-17 (age may vary by region) */
    Tn,
    /** Adult age group for users ages 18 and up (age may vary by region) */
    Ad,
};

ovrAccountAgeCategory ConvertAccountAgeCategory(EOvrAccountAgeCategory Value);
EOvrAccountAgeCategory ConvertAccountAgeCategory(ovrAccountAgeCategory Value);

/** AchievementType enumeration. */
UENUM(BlueprintType)
enum class EOvrAchievementType : uint8
{
    Unknown,
    Simple,
    Bitfield,
    Count,
};

ovrAchievementType ConvertAchievementType(EOvrAchievementType Value);
EOvrAchievementType ConvertAchievementType(ovrAchievementType Value);

/** Age category for developers to send to Meta. The values are used in UserAgeCategory_Report() API. */
UENUM(BlueprintType)
enum class EOvrAppAgeCategory : uint8
{
    Unknown,
    /** Child age group for users between the ages of 10-12 (age may vary by region) */
    Ch,
    /** Non-child age group for users ages 13 and up (age may vary by region) */
    Nch,
};

ovrAppAgeCategory ConvertAppAgeCategory(EOvrAppAgeCategory Value);
EOvrAppAgeCategory ConvertAppAgeCategory(ovrAppAgeCategory Value);

/**
 * Result of installing an app. In case of an error during install process,
 * the error message contains the string representation of this result. This is
 * returned from Application_StartAppDownload(), Application_CancelAppDownload() and Application_InstallAppUpdateAndRelaunch() APIs.
 */
UENUM(BlueprintType)
enum class EOvrAppInstallResult : uint8
{
    Unknown,
    /** Install of the app failed due to low storage on the device */
    LowStorage,
    /** Install of the app failed due to a network error */
    NetworkError,
    /**
     * Install of the app failed as another install request for this application
     * is already being processed by the installer
     */
    DuplicateRequest,
    /** Install of the app failed due to an internal installer error */
    InstallerError,
    /** Install of the app failed because the user cancelled the install operation */
    UserCancelled,
    /** Install of the app failed due to a user authorization error */
    AuthorizationError,
    /** Install of the app succeeded */
    Success,
};

ovrAppInstallResult ConvertAppInstallResult(EOvrAppInstallResult Value);
EOvrAppInstallResult ConvertAppInstallResult(ovrAppInstallResult Value);

/**
 * Current status of the app on the device. An app can only check
 * its own status.
 */
UENUM(BlueprintType)
enum class EOvrAppStatus : uint8
{
    Unknown,
    /** User has valid entitlement to the app but it is not currently installed on the device. */
    Entitled,
    /** Download of the app is currently queued. */
    DownloadQueued,
    /** Download of the app is currently in progress. */
    Downloading,
    /** Install of the app is currently in progress. */
    Installing,
    /** App is installed on the device. */
    Installed,
    /** App is being uninstalled from the device. */
    Uninstalling,
    /** Install of the app is currently queued. */
    InstallQueued,
};

ovrAppStatus ConvertAppStatus(EOvrAppStatus Value);
EOvrAppStatus ConvertAppStatus(ovrAppStatus Value);

/** ChallengeCreationType enumeration. */
UENUM(BlueprintType)
enum class EOvrChallengeCreationType : uint8
{
    Unknown,
    UserCreated,
    DeveloperCreated,
};

ovrChallengeCreationType ConvertChallengeCreationType(EOvrChallengeCreationType Value);
EOvrChallengeCreationType ConvertChallengeCreationType(ovrChallengeCreationType Value);

/** ChallengeViewerFilter enumeration. */
UENUM(BlueprintType)
enum class EOvrChallengeViewerFilter : uint8
{
    Unknown,
    AllVisible,
    Participating,
    Invited,
    ParticipatingOrInvited,
};

ovrChallengeViewerFilter ConvertChallengeViewerFilter(EOvrChallengeViewerFilter Value);
EOvrChallengeViewerFilter ConvertChallengeViewerFilter(ovrChallengeViewerFilter Value);

/** ChallengeVisibility enumeration. */
UENUM(BlueprintType)
enum class EOvrChallengeVisibility : uint8
{
    Unknown,
    /** Only those invited can participate in it. Everyone can see it */
    InviteOnly,
    /** Everyone can participate and see this challenge */
    Public,
    /** Only those invited can participate and see this challenge */
    Private,
};

ovrChallengeVisibility ConvertChallengeVisibility(EOvrChallengeVisibility Value);
EOvrChallengeVisibility ConvertChallengeVisibility(ovrChallengeVisibility Value);

/** KeyValuePairType enumeration. */
UENUM(BlueprintType)
enum class EOvrKeyValuePairType : uint8
{
    String,
    Int,
    Double,
    Unknown,
};

ovrKeyValuePairType ConvertKeyValuePairType(EOvrKeyValuePairType Value);
EOvrKeyValuePairType ConvertKeyValuePairType(ovrKeyValuePairType Value);

/** LaunchResult enumeration. */
UENUM(BlueprintType)
enum class EOvrLaunchResult : uint8
{
    Unknown,
    Success,
    FailedRoomFull,
    FailedGameAlreadyStarted,
    FailedRoomNotFound,
    FailedUserDeclined,
    FailedOtherReason,
};

ovrLaunchResult ConvertLaunchResult(EOvrLaunchResult Value);
EOvrLaunchResult ConvertLaunchResult(ovrLaunchResult Value);

/** LaunchType enumeration. */
UENUM(BlueprintType)
enum class EOvrLaunchType : uint8
{
    Unknown,
    /**  Normal launch from the user's library  */
    Normal,
    /**
     *  Launch from the user accepting an invite.  Check field FOvrLaunchDetails::LobbySessionID,
     *     field FOvrLaunchDetails::MatchSessionID, field FOvrLaunchDetails::DestinationApiName and
     *     field FOvrLaunchDetails::DeeplinkMessage. 
     */
    Invite,
    /**  DEPRECATED  */
    Coordinated,
    /**
     *  Launched from Application_LaunchOtherApp().
     *     Check field FOvrLaunchDetails::LaunchSource and field FOvrLaunchDetails::DeeplinkMessage. 
     */
    Deeplink,
};

ovrLaunchType ConvertLaunchType(EOvrLaunchType Value);
EOvrLaunchType ConvertLaunchType(ovrLaunchType Value);

/** LeaderboardFilterType enumeration. */
UENUM(BlueprintType)
enum class EOvrLeaderboardFilterType : uint8
{
    None,
    Friends,
    Unknown,
    UserIds,
};

ovrLeaderboardFilterType ConvertLeaderboardFilterType(EOvrLeaderboardFilterType Value);
EOvrLeaderboardFilterType ConvertLeaderboardFilterType(ovrLeaderboardFilterType Value);

/** LeaderboardStartAt enumeration. */
UENUM(BlueprintType)
enum class EOvrLeaderboardStartAt : uint8
{
    Top,
    CenteredOnViewer,
    CenteredOnViewerOrTop,
    Unknown,
};

ovrLeaderboardStartAt ConvertLeaderboardStartAt(EOvrLeaderboardStartAt Value);
EOvrLeaderboardStartAt ConvertLeaderboardStartAt(ovrLeaderboardStartAt Value);

/** LivestreamingStartStatus enumeration. */
UENUM(BlueprintType)
enum class EOvrLivestreamingStartStatus : uint8
{
    Success = 1,
    Unknown = 0,
    NoPackageSet = uint8(-1),
    NoFbConnect = uint8(-2),
    NoSessionId = uint8(-3),
    MissingParameters = uint8(-4),
};

ovrLivestreamingStartStatus ConvertLivestreamingStartStatus(EOvrLivestreamingStartStatus Value);
EOvrLivestreamingStartStatus ConvertLivestreamingStartStatus(ovrLivestreamingStartStatus Value);

/** LogEventName enumeration. */
UENUM(BlueprintType)
enum class EOvrLogEventName : uint8
{
    Unknown,
    AdClick,
    AdImpression,
    VrCompleteRegistration,
    VrTutorialCompletion,
    Contact,
    CustomizeProduct,
    Donate,
    FindLocation,
    VrRate,
    Schedule,
    VrSearch,
    SmartTrial,
    SubmitApplication,
    Subscribe,
    VrContentView,
    VrSdkInitialize,
    VrSdkBackgroundStatusAvailable,
    VrSdkBackgroundStatusDenied,
    VrSdkBackgroundStatusRestricted,
    VrAddPaymentInfo,
    VrAddToCart,
    VrAddToWishlist,
    VrInitiatedCheckout,
    VrPurchase,
    VrCatalogUpdate,
    VrPurchaseFailed,
    VrPurchaseRestored,
    SubscriptionInitiatedCheckout,
    SubscriptionFailed,
    SubscriptionRestore,
    VrLevelAchieved,
    VrAchievementUnlocked,
    VrSpentCredits,
    VrObtainPushToken,
    VrPushOpened,
    VrActivateApp,
    VrDeactivateApp,
};

ovrLogEventName ConvertLogEventName(EOvrLogEventName Value);
EOvrLogEventName ConvertLogEventName(ovrLogEventName Value);

/** LogEventParameter enumeration. */
UENUM(BlueprintType)
enum class EOvrLogEventParameter : uint8
{
    Unknown,
    VrCurrency,
    VrRegistrationMethod,
    VrContentType,
    VrContent,
    VrContentId,
    VrSearchString,
    VrSuccess,
    VrMaxRatingValue,
    VrPaymentInfoAvailable,
    VrNumItems,
    VrLevel,
    VrDescription,
    AdType,
    VrOrderId,
    EventName,
    LogTime,
    ImplicitlyLogged,
    InBackground,
    VrPushCampaign,
    VrPushAction,
    VrIapProductType,
    VrContentTitle,
    VrTransactionId,
    VrTransactionDate,
    VrIapSubsPeriod,
    VrIapIsStartTrial,
    VrIapHasFreeTrial,
    VrIapTrialPeriod,
    VrIapTrialPrice,
    SessionId,
};

ovrLogEventParameter ConvertLogEventParameter(EOvrLogEventParameter Value);
EOvrLogEventParameter ConvertLogEventParameter(ovrLogEventParameter Value);

/** MediaContentType enumeration. */
UENUM(BlueprintType)
enum class EOvrMediaContentType : uint8
{
    Unknown,
    Photo,
};

ovrMediaContentType ConvertMediaContentType(EOvrMediaContentType Value);
EOvrMediaContentType ConvertMediaContentType(ovrMediaContentType Value);

/** MultiplayerErrorErrorKey enumeration. */
UENUM(BlueprintType)
enum class EOvrMultiplayerErrorErrorKey : uint8
{
    Unknown,
    DestinationUnavailable,
    DlcRequired,
    General,
    GroupFull,
    InviterNotJoinable,
    LevelNotHighEnough,
    LevelNotUnlocked,
    NetworkTimeout,
    NoLongerAvailable,
    UpdateRequired,
    TutorialRequired,
};

ovrMultiplayerErrorErrorKey ConvertMultiplayerErrorErrorKey(EOvrMultiplayerErrorErrorKey Value);
EOvrMultiplayerErrorErrorKey ConvertMultiplayerErrorErrorKey(ovrMultiplayerErrorErrorKey Value);

/** NetSyncConnectionStatus enumeration. */
UENUM(BlueprintType)
enum class EOvrNetSyncConnectionStatus : uint8
{
    Unknown,
    Connecting,
    Disconnected,
    Connected,
};

ovrNetSyncConnectionStatus ConvertNetSyncConnectionStatus(EOvrNetSyncConnectionStatus Value);
EOvrNetSyncConnectionStatus ConvertNetSyncConnectionStatus(ovrNetSyncConnectionStatus Value);

/** NetSyncDisconnectReason enumeration. */
UENUM(BlueprintType)
enum class EOvrNetSyncDisconnectReason : uint8
{
    Unknown,
    /** when disconnect was requested */
    LocalTerminated,
    /** server intentionally closed the connection */
    ServerTerminated,
    /** initial connection never succeeded */
    Failed,
    /** network timeout */
    Lost,
};

ovrNetSyncDisconnectReason ConvertNetSyncDisconnectReason(EOvrNetSyncDisconnectReason Value);
EOvrNetSyncDisconnectReason ConvertNetSyncDisconnectReason(ovrNetSyncDisconnectReason Value);

/** NetSyncVoipMicSource enumeration. */
UENUM(BlueprintType)
enum class EOvrNetSyncVoipMicSource : uint8
{
    Unknown,
    None,
    Internal,
};

ovrNetSyncVoipMicSource ConvertNetSyncVoipMicSource(EOvrNetSyncVoipMicSource Value);
EOvrNetSyncVoipMicSource ConvertNetSyncVoipMicSource(ovrNetSyncVoipMicSource Value);

/** NetSyncVoipStreamMode enumeration. */
UENUM(BlueprintType)
enum class EOvrNetSyncVoipStreamMode : uint8
{
    Unknown,
    Ambisonic,
    Mono,
};

ovrNetSyncVoipStreamMode ConvertNetSyncVoipStreamMode(EOvrNetSyncVoipStreamMode Value);
EOvrNetSyncVoipStreamMode ConvertNetSyncVoipStreamMode(ovrNetSyncVoipStreamMode Value);

/** PartyUpdateAction enumeration. */
UENUM(BlueprintType)
enum class EOvrPartyUpdateAction : uint8
{
    Unknown,
    Join,
    Leave,
    Invite,
    Uninvite,
};

ovrPartyUpdateAction ConvertPartyUpdateAction(EOvrPartyUpdateAction Value);
EOvrPartyUpdateAction ConvertPartyUpdateAction(ovrPartyUpdateAction Value);

/** PermissionGrantStatus enumeration. */
UENUM(BlueprintType)
enum class EOvrPermissionGrantStatus : uint8
{
    Unknown,
    Granted,
    Denied,
    Blocked,
};

ovrPermissionGrantStatus ConvertPermissionGrantStatus(EOvrPermissionGrantStatus Value);
EOvrPermissionGrantStatus ConvertPermissionGrantStatus(ovrPermissionGrantStatus Value);

/**
 * Describes the various results possible when attempting to initialize the platform.
 * Anything other than ovrPlatformInitialize_Success should be considered a fatal error
 * with respect to using the platform, as the platform is not guaranteed to be legitimate
 * or work correctly.
 */
UENUM(BlueprintType)
enum class EOvrPlatformInitializeResult : uint8
{
    Success = 0,
    Uninitialized = uint8(-1),
    PreLoaded = uint8(-2),
    FileInvalid = uint8(-3),
    SignatureInvalid = uint8(-4),
    UnableToVerify = uint8(-5),
    VersionMismatch = uint8(-6),
    Unknown = uint8(-7),
    InvalidCredentials = uint8(-8),
    NotEntitled = uint8(-9),
};

ovrPlatformInitializeResult ConvertPlatformInitializeResult(EOvrPlatformInitializeResult Value);
EOvrPlatformInitializeResult ConvertPlatformInitializeResult(ovrPlatformInitializeResult Value);

/** Possible states that an app can respond to the platform notification that the in-app reporting flow has been requested by the user. */
UENUM(BlueprintType)
enum class EOvrReportRequestResponse : uint8
{
    Unknown,
    /** Response to the platform notification that the in-app reporting flow request is handled. */
    Handled,
    /** Response to the platform notification that the in-app reporting flow request is not handled. */
    Unhandled,
    /** Response to the platform notification that the in-app reporting flow is unavailable or non-existent. */
    Unavailable,
};

ovrReportRequestResponse ConvertReportRequestResponse(EOvrReportRequestResponse Value);
EOvrReportRequestResponse ConvertReportRequestResponse(ovrReportRequestResponse Value);

/** Display extra information about the user's presence */
UENUM(BlueprintType)
enum class EOvrRichPresenceExtraContext : uint8
{
    Unknown,
    /** Display nothing */
    None,
    /** Display the current amount with the user over the max */
    CurrentCapacity,
    /** Display how long ago the match/game/race/etc started */
    StartedAgo,
    /** Display how soon the match/game/race/etc will end */
    EndingIn,
    /** Display that this user is looking for a match */
    LookingForAMatch,
};

ovrRichPresenceExtraContext ConvertRichPresenceExtraContext(EOvrRichPresenceExtraContext Value);
EOvrRichPresenceExtraContext ConvertRichPresenceExtraContext(ovrRichPresenceExtraContext Value);

/** SdkAccountType enumeration. */
UENUM(BlueprintType)
enum class EOvrSdkAccountType : uint8
{
    Unknown,
    Oculus,
    FacebookGameroom,
};

ovrSdkAccountType ConvertSdkAccountType(EOvrSdkAccountType Value);
EOvrSdkAccountType ConvertSdkAccountType(ovrSdkAccountType Value);

/** ServiceProvider enumeration. */
UENUM(BlueprintType)
enum class EOvrServiceProvider : uint8
{
    Unknown,
    Dropbox,
    Facebook,
    Google,
    Instagram,
    RemoteMedia,
};

ovrServiceProvider ConvertServiceProvider(EOvrServiceProvider Value);
EOvrServiceProvider ConvertServiceProvider(ovrServiceProvider Value);

/** ShareMediaStatus enumeration. */
UENUM(BlueprintType)
enum class EOvrShareMediaStatus : uint8
{
    Unknown,
    Shared,
    Canceled,
};

ovrShareMediaStatus ConvertShareMediaStatus(EOvrShareMediaStatus Value);
EOvrShareMediaStatus ConvertShareMediaStatus(ovrShareMediaStatus Value);

/** SystemVoipStatus enumeration. */
UENUM(BlueprintType)
enum class EOvrSystemVoipStatus : uint8
{
    Unknown,
    Unavailable,
    Suppressed,
    Active,
};

ovrSystemVoipStatus ConvertSystemVoipStatus(EOvrSystemVoipStatus Value);
EOvrSystemVoipStatus ConvertSystemVoipStatus(ovrSystemVoipStatus Value);

/** How far should we go back in time looking at history?  This is used in some requests such as User_GetLoggedInUserRecentlyMetUsersAndRooms() */
UENUM(BlueprintType)
enum class EOvrTimeWindow : uint8
{
    Unknown,
    OneHour,
    OneDay,
    OneWeek,
    ThirtyDays,
    NinetyDays,
};

ovrTimeWindow ConvertTimeWindow(EOvrTimeWindow Value);
EOvrTimeWindow ConvertTimeWindow(ovrTimeWindow Value);

/**
 * The ordering that is used when returning a list of users. This is used in
 * some requests such as Room_GetInvitableUsers2()
 */
UENUM(BlueprintType)
enum class EOvrUserOrdering : uint8
{
    Unknown,
    /** No preference for ordering (could be in any or no order) */
    None,
    /**
     * Orders by online users first and then offline users. Within each group the
     * users are ordered alphabetically by display name
     */
    PresenceAlphabetical,
};

ovrUserOrdering ConvertUserOrdering(EOvrUserOrdering Value);
EOvrUserOrdering ConvertUserOrdering(ovrUserOrdering Value);

/** UserPresenceStatus enumeration. */
UENUM(BlueprintType)
enum class EOvrUserPresenceStatus : uint8
{
    Unknown,
    Online,
    Offline,
};

ovrUserPresenceStatus ConvertUserPresenceStatus(EOvrUserPresenceStatus Value);
EOvrUserPresenceStatus ConvertUserPresenceStatus(ovrUserPresenceStatus Value);

/** VoipBitrate enumeration. */
UENUM(BlueprintType)
enum class EOvrVoipBitrate : uint8
{
    Unknown,
    /**
     * Very low audio quality for minimal network usage. This may not give the full range of Hz for
     * audio, but it will save on network usage.
     */
    B16000,
    /** Lower audio quality but also less network usage. */
    B24000,
    /**
     * This is the default bitrate for voip connections. It should be the best tradeoff between
     * audio quality and network usage.
     */
    B32000,
    /**
     * Higher audio quality at the expense of network usage. Good if there's music being streamed
     * over the connections
     */
    B64000,
    /** Even higher audio quality for music streaming or radio-like quality. */
    B96000,
    /**
     * At this point the audio quality should be preceptually indistinguishable from the uncompressed
     * input.
     */
    B128000,
};

ovrVoipBitrate ConvertVoipBitrate(EOvrVoipBitrate Value);
EOvrVoipBitrate ConvertVoipBitrate(ovrVoipBitrate Value);

/** VoipDtxState enumeration. */
UENUM(BlueprintType)
enum class EOvrVoipDtxState : uint8
{
    Unknown,
    Enabled,
    Disabled,
};

ovrVoipDtxState ConvertVoipDtxState(EOvrVoipDtxState Value);
EOvrVoipDtxState ConvertVoipDtxState(ovrVoipDtxState Value);

/** VoipMuteState enumeration. */
UENUM(BlueprintType)
enum class EOvrVoipMuteState : uint8
{
    Unknown,
    Muted,
    Unmuted,
};

ovrVoipMuteState ConvertVoipMuteState(EOvrVoipMuteState Value);
EOvrVoipMuteState ConvertVoipMuteState(ovrVoipMuteState Value);

/** VoipSampleRate enumeration. */
UENUM(BlueprintType)
enum class EOvrVoipSampleRate : uint8
{
    Unknown,
    HZ24000,
    HZ44100,
    HZ48000,
};

ovrVoipSampleRate ConvertVoipSampleRate(EOvrVoipSampleRate Value);
EOvrVoipSampleRate ConvertVoipSampleRate(ovrVoipSampleRate Value);
