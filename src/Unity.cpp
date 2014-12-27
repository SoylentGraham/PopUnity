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
	
	auto& App = PopUnity::Get();
	App.PushJob( JobAndChannel );
}

std::shared_ptr<TJob> PopUnity::PopJob()
{
	if ( mPendingJobs.IsEmpty() )
		return nullptr;
	
	return mPendingJobs.PopAt(0);
}

void PopUnity::PushJob(TJobAndChannel& JobAndChannel)
{
	std::shared_ptr<TJob> Job( new TJob(JobAndChannel.GetJob() ) );
	mPendingJobs.PushBack( Job );
	
}

TJobInterfaceWrapper::TJobInterfaceWrapper(const TJob& Job)
{
	//	construct
	mTJob = &Job;
	mCommand = Job.mParams.mCommand.c_str();
	mError = nullptr;
	mParamCount = 0;

	//
	auto ErrorParam = Job.mParams.GetErrorParam();
	if ( ErrorParam.IsValid() )
	{
		//	gr: todo: no magic strings!
		mError = mStringBuffer.PushBack(Job.mParams.GetParamAs<std::string>("error")).c_str();
	}
	
	auto& Params = Job.mParams.mParams;
	for ( int i=0;	i<Params.GetSize();	i++ )
	{
		mParamNames[mParamCount] = Params[i].mName.c_str();
		mParamCount++;
	}
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
	Job.mChannelMeta.mChannelRef = Channel->GetChannelRef();
	if ( !Channel->SendCommand( Job ) )
		return false;
	
	return true;
}

extern "C" bool EXPORT_API PopJob(Unity::JobCallback Func)
{
	auto& App = PopUnity::Get();
	
	auto Job = App.PopJob();
	if ( !Job )
		return false;
	
	//	make a temp interface for c#
	TJobInterfaceWrapper JobInterface( *Job.get() );
	
	//	send job interface to c# blocking callback
	auto& Interface = static_cast<TJobInterface&>( JobInterface );
	Func( &Interface );
	
	//	done, dispose of job
	Job.reset();
	
	return true;
}

