#pragma once

#include <ofxSoylent.h>
#include <TJob.h>
#include <TChannel.h>



// If exported by a plugin, this function will be called for GL.IssuePluginEvent script calls.
// The function will be called on a rendering thread; note that when multithreaded rendering is used,
// the rendering thread WILL BE DIFFERENT from the thread that all scripts & other game logic happens!
// You have to ensure any synchronization with other plugin script calls is properly done by you.
extern "C" void EXPORT_API UnityRenderEvent(int eventID);


class PopUnity : public TChannelManager
{
public:
	PopUnity();
};

extern "C" uint64 EXPORT_API CreateChannel(const char* ChannelSpec);
extern "C" uint64 EXPORT_API Test();
