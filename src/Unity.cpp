#include "Unity.h"
#include <PopMain.h>


std::shared_ptr<PopUnity> gApp;

PopUnity& GetApp()
{
	if ( !gApp )
	{
		gApp.reset( new PopUnity );
	}
	return *gApp;
}


TPopAppError::Type PopMain(TJobParams& Params)
{
	//	unused in dll?
	return TPopAppError::Success;
}

PopUnity::PopUnity()
{
	
}

extern "C" void EXPORT_API UnityRenderEvent(int eventID)
{
	switch ( eventID )
	{
	}
}

extern "C" uint64 EXPORT_API CreateChannel(const char* ChannelSpec)
{
	static SoyRef ChannelRef("chan");
	ChannelRef.Increment();
	
	auto Channel = CreateChannelFromInputString( ChannelSpec, ChannelRef );
	if ( !Channel )
		return 0;
	
	auto& App = GetApp();
	App.AddChannel( Channel );
	return Channel->GetChannelRef().GetInt64();
}

extern "C" uint64 EXPORT_API Test()
{
	return 1234;
}
