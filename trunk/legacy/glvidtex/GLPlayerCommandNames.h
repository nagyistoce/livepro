#ifndef GLPlayerCommandNames_H
#define GLPlayerCommandNames_H

// login/setup
#define GLPlayer_Login "Login"
#define GLPlayer_Ping "Ping"
#define GLPlayer_ListVideoInputs "ListVideoInputs"

// player operation
#define GLPlayer_SetBlackout "SetBlackout"
#define GLPlayer_SetCrossfadeSpeed "SetCrossfadeSpeed"

#define GLPlayer_PreloadSlideGroup "PreloadSlideGroup"
#define GLPlayer_LoadSlideGroup "LoadSlideGroup"
#define GLPlayer_SetSlide "SetSlide"
#define GLPlayer_SetLayout "SetLayout"
#define GLPlayer_SetUserProperty "SetUserProperty"
#define GLPlayer_SetVisibility "SetVisibility"
#define GLPlayer_DownloadFile "DownloadFile"
#define GLPlayer_QueryProperty "QueryProperty"
#define GLPlayer_QueryLayout "QueryLayout"

// basic output config
#define GLPlayer_SetIgnoreAR "SetIgnoreAR"
#define GLPlayer_SetViewport "SetViewport"
#define GLPlayer_SetCanvasSize "SetCanvasSize"
#define GLPlayer_SetScreen "SetScreen"

// subviews
#define GLPlayer_AddSubview "AddSubview"
#define GLPlayer_RemoveSubview "RemoveSubview"
#define GLPlayer_ClearSubviews "ClearSubviews"

// playlist commands
#define GLPlayer_SetPlaylistPlaying "SetPlaylistPlaying"
#define GLPlayer_UpdatePlaylist "UpdatePlaylist"
#define GLPlayer_SetPlaylistTime "SetPlaylistTime"
// The next 2 are sent FROM the player TO the director
#define GLPlayer_PlaylistTimeChanged "PlaylistTimeChanged"
#define GLPlayer_CurrentPlaylistItemChanged "CurrentPlaylistItemChanged"

// overlays
#define GLPlayer_AddOverlay "AddOverlay"
#define GLPlayer_RemoveOverlay "RemoveOverlay"

/// Right now, the following are only used by the json server

// Give a tree list of all the Groups (name/ids), scenes in each group (name/ids), drawables in each scene (names/ids), props on each drawable (name, type, value)
#define GLPlayer_ExamineCollection	 "ExamineCollection"
// Using the list of group IDs from GLPlayer_ExamineCollection, load a specific group from the collection file as the current group
#define GLPlayer_LoadGroupFromCollection "LoadGroupFromCollection"
// Give a tree list of all scenes in current group (name/ids), drawables in each scene (names/ids), props on each drawable (name, type, value)
#define GLPlayer_ExamineCurrentGroup	 "ExamineCurrentGroup"
// Give a tree list of all drawables in current scene (names/ids), props on each drawable (name, type, value)
#define GLPlayer_ExamineCurrentScene	 "ExamineCurrentScene"
// Using the sceneId from one of the Examine commands above, set a property on the scene such as duration, datetime, etc
#define GLPlayer_SetSceneProperty	 "SetSceneProperty"
// Using the ID of a playlist item and a drawableid, remove the item from the drawable's playlist
#define GLPlayer_RemovePlaylistItem	 "RemovePlaylistItem"
// Add an item to a drawables playlist
#define GLPlayer_AddPlaylistItem	 "AddPlaylistItem"
// Write the currently loaded collection (loaded using LoadGroupFromCollection) to a file
#define GLPlayer_WriteCollection	 "WriteCollection"

// TODO not implemented below, should we implement these since they are all in subviews?
#define GLPlayer_SetAlphaMask "SetAlphaMask"
#define GLPlayer_SetKeystone "SetKeystone"
#define GLPlayer_SetWindow "SetWindow"
#define GLPlayer_SetFlip "SetFlip"
#define GLPlayer_SetColors "SetColors"

#endif
