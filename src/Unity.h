#pragma once

#include <ofxSoylent.h>
#include <TJob.h>
#include <TChannel.h>



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

namespace UnityEvent
{
	//	gl.IssuePluginEvent event id's
	enum Type
	{
		PostRender = 0,		
	};
	
	extern SoyEvent<int>	mOnPostRender;
	extern SoyEvent<bool>	mOnStopped;		//	editor stopped
};

namespace Unity
{
	typedef void (*LogCallback)(const char*);
	typedef void (*JobCallback)(const TJobInterface*);
};


class TCopyTextureCommand
{
public:
	std::shared_ptr<SoyData_Impl<SoyPixels>>	mPixels;
	int											mTexture;
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
	void			OnStopped();
	
	std::shared_ptr<TJob>	PopJob();
	void			PushJob(TJobAndChannel& JobAndChannel);
	
	void			CopyTexture(std::shared_ptr<SoyData_Impl<SoyPixels>> Pixels,int Texture);
	void			ProcessCopyTextureQueue();
	
private:
	void			OnJobRecieved(TJobAndChannel& JobAndChannel);	//	special job handling to send back to unity
	
private:
	Array<std::shared_ptr<TJob>>	mPendingJobs;
	std::mutex						mDebugMessagesLock;
	Array<std::string>				mDebugMessages;	//	gr: might need to be threadsafe
	ofMutexT<Array<TCopyTextureCommand>>	mCopyTextureQueue;
};


