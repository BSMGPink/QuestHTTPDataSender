param (
    [Parameter(Mandatory=$false)]
    [Switch]$useDebug,
    [Parameter(Mandatory=$false)]
    [Switch]$log
)

& ./build.ps1
if ($useDebug.IsPresent) {
    & adb push build/debug/libdatasender.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libdatasender.so
} else {
    & adb push build/libdatasender.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libdatasender.so
}

& adb shell am force-stop com.beatgames.beatsaber
& adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity
if ($log.IsPresent) {
    & ./log.ps1
}