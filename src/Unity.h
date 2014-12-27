#pragma once

#include <ofxSoylent.h>
#include <TJob.h>
#include <TChannel.h>



// If exported by a plugin, this function will be called for GL.IssuePluginEvent script calls.
// The function will be called on a rendering thread; note that when multithreaded rendering is used,
// the rendering thread WILL BE DIFFERENT from the thread that all scripts & other game logic happens!
// You have to ensure any synchronization with other plugin script calls is properly done by you.
extern "C" void EXPORT_API UnityRenderEvent(int eventID);


//	c# struct
const int MaxCSharpParams = 10;
typedef struct
{
	const TJob*		mTJob;		//	pointer to job
	const char*		mCommand;
	const char*		mError;		//	common
	uint32			mParamCount;
	const char*		mParamNames[MaxCSharpParams];
	
} TJobInterface;

class TJobInterfaceWrapper : public TJobInterface
{
public:
	TJobInterfaceWrapper(const TJob& Job);
	
private:
	BufferArray<std::string,MaxCSharpParams*2+1> mStringBuffer;
};


namespace Unity
{
	typedef void (*LogCallback)(const char*);
	typedef void (*JobCallback)(const TJobInterface*);
};

class PopUnity : public TChannelManager
{
public:
	static PopUnity&	Get();		//	get singleton
	
public:
	PopUnity();

	virtual void	AddChannel(std::shared_ptr<TChannel> Channel) override;

	void			FlushDebugMessages(void (*LogFunc)(const char*));
	void			OnDebug(const std::string& Debug);
	
	std::shared_ptr<TJob>	PopJob();
	void			PushJob(TJobAndChannel& JobAndChannel);
	
private:
	void			OnJobRecieved(TJobAndChannel& JobAndChannel);	//	special job handling to send back to unity
	
private:
	Array<std::shared_ptr<TJob>>	mPendingJobs;
	Array<std::string>				mDebugMessages;	//	gr: might need to be threadsafe
};



extern "C" void EXPORT_API FlushDebug(Unity::LogCallback LogFunc)
{
	PopUnity::Get().FlushDebugMessages(LogFunc);
}
