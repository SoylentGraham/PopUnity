#include "Unity.h"
#include <PopMain.h>


std::shared_ptr<PopUnity> gApp;


TPopAppError::Type PopMain(TJobParams& Params)
{
	//	unused in dll?
	return TPopAppError::Success;
}


PopUnity& PopUnity::Get()
{
	if ( !gApp )
	{
		gApp.reset( new PopUnity );
	}
	return *gApp;
}


PopUnity::PopUnity()
{
	std::Debug.GetOnFlushEvent().AddListener( *this, &PopUnity::OnDebug );
}

void PopUnity::OnDebug(const std::string& Debug)
{
	mDebugMessages.PushBack( Debug );
	
	//	gr: cap this in case it's never flushed...
	const int MessageLimit = 300;
	if ( mDebugMessages.GetSize() > MessageLimit )
	{
		int CullCount = mDebugMessages.GetSize() - MessageLimit;
		mDebugMessages.RemoveBlock( 0, CullCount );
		std::stringstream CullDebug;
		CullDebug << "Culled " << CullCount << " debug messages";
		mDebugMessages.PushBack( CullDebug.str() );
	}
}


void PopUnity::FlushDebugMessages(void (*LogFunc)(const char*))
{
	//	send all messages to a delegate
	//	gr: FIFO is inefficient, fix this, but still display in order...
	while ( !mDebugMessages.IsEmpty() )
	{
		if ( LogFunc )
			LogFunc( mDebugMessages[0].c_str() );
		mDebugMessages.PopAt(0);
	}
}


void PopUnity::AddChannel(std::shared_ptr<TChannel> Channel)
{
	TChannelManager::AddChannel( Channel );
	Channel->mOnJobRecieved.AddListener( *this, &PopUnity::OnJobRecieved );
}

void PopUnity::OnJobRecieved(TJobAndChannel& JobAndChannel)
{
	//	send back to appropriate C# delegate!
	std::Debug << "OnJobRecieved; " << JobAndChannel.GetJob().mParams.mCommand << " from " << JobAndChannel.GetChannel().GetChannelRef() << std::endl;
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
	
	auto& App = PopUnity::Get();
	App.AddChannel( Channel );
	return Channel->GetChannelRef().GetInt64();
}

extern "C" bool EXPORT_API SendJob(uint64 ChannelRef,const char* Command)
{
	auto& App = PopUnity::Get();
	auto Channel = App.GetChannel( SoyRef(ChannelRef) );
	if ( !Channel )
		return false;
	
	TJob Job;
	Job.mParams.mCommand = Command;
	if ( !Channel->SendCommand( Job ) )
		return false;
	
	return true;
}

extern "C" uint64 EXPORT_API Test()
{
	return 1234;
}
