#ifndef VideoSenderCommands_H
#define VideoSenderCommands_H

// For the next three "set" commands, a single argument
// expected, called "value", type "int", as a percent - e.g. {value:50} means 50%
#define Video_SetHue "SetHue"
#define Video_SetSaturation "SetSaturation"
#define Video_SetBright "SetBright"
#define Video_SetContrast "SetContrast"

#define Video_GetHue "GetHue"
#define Video_GetSaturation "GetSaturation"
#define Video_GetBright "GetBright"
#define Video_GetContrast "GetContrast"

// FPS: Arg 'fps', type int
#define Video_SetFPS "SetFPS"
#define Video_GetFPS "GetFPS"
// Size: Arg 'w', type 'int', Arg 'h', type 'int'
#define Video_SetSize "SetSize"
#define Video_GetSize "GetSize"

#endif
