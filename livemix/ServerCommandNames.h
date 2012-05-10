#ifndef ServerCommandNames_H
#define ServerCommandNames_H

// login/setup
#define Server_Login "Login"
#define Server_Ping "Ping"
#define Server_ListVideoInputs "ListVideoInputs"

// player operation
#define Server_SetBlackout "SetBlackout"
#define Server_SetCrossfadeSpeed "SetCrossfadeSpeed"

#define Server_PreloadSlideGroup "PreloadSlideGroup"
#define Server_LoadSlideGroup "LoadSlideGroup"
#define Server_SetSlide "SetSlide"
#define Server_SetLayout "SetLayout"
#define Server_SetUserProperty "SetUserProperty"
#define Server_SetVisibility "SetVisibility"
#define Server_DownloadFile "DownloadFile"
#define Server_QueryProperty "QueryProperty"
#define Server_QueryLayout "QueryLayout"

// basic output config
#define Server_SetIgnoreAR "SetIgnoreAR"
#define Server_SetViewport "SetViewport"
#define Server_SetCanvasSize "SetCanvasSize"
#define Server_SetScreen "SetScreen"

// subviews
#define Server_AddSubview "AddSubview"
#define Server_RemoveSubview "RemoveSubview"
#define Server_ClearSubviews "ClearSubviews"

// playlist commands
#define Server_SetPlaylistPlaying "SetPlaylistPlaying"
#define Server_UpdatePlaylist "UpdatePlaylist"
#define Server_SetPlaylistTime "SetPlaylistTime"
// The next 2 are sent FROM the player TO the director
#define Server_PlaylistTimeChanged "PlaylistTimeChanged"
#define Server_CurrentPlaylistItemChanged "CurrentPlaylistItemChanged"

// overlays
#define Server_AddOverlay "AddOverlay"
#define Server_RemoveOverlay "RemoveOverlay"

/// Right now, the following are only used by the json server

// Give a tree list of all the Groups (name/ids), scenes in each group (name/ids), drawables in each scene (names/ids), props on each drawable (name, type, value)
#define Server_ExamineCollection	 "ExamineCollection"
// Using the list of group IDs from Server_ExamineCollection, load a specific group from the collection file as the current group
#define Server_LoadGroupFromCollection "LoadGroupFromCollection"
// Give a tree list of all scenes in current group (name/ids), drawables in each scene (names/ids), props on each drawable (name, type, value)
#define Server_ExamineCurrentGroup	 "ExamineCurrentGroup"
// Give a tree list of all drawables in current scene (names/ids), props on each drawable (name, type, value)
#define Server_ExamineCurrentScene	 "ExamineCurrentScene"
// Using the sceneId from one of the Examine commands above, set a property on the scene such as duration, datetime, etc
#define Server_SetSceneProperty	 "SetSceneProperty"
// Using the ID of a playlist item and a drawableid, remove the item from the drawable's playlist
#define Server_RemovePlaylistItem	 "RemovePlaylistItem"
// Add an item to a drawables playlist
#define Server_AddPlaylistItem	 	"AddPlaylistItem"
// Write the currently loaded collection (loaded using LoadGroupFromCollection) to a file
#define Server_WriteCollection	 	"WriteCollection"
// Render a video connection string 'con' on the screen using a GLVideoInputDrawable
#define Server_ShowVideoConnection	 "ShowVideoConnection"

/// For detecting dead connections by the client...
#define Server_DeadPing "DeadPing"

// TODO not implemented below, should we implement these since they are all in subviews?
#define Server_SetAlphaMask "SetAlphaMask"
#define Server_SetKeystone "SetKeystone"
#define Server_SetWindow "SetWindow"
#define Server_SetFlip "SetFlip"
#define Server_SetColors "SetColors"

#endif
