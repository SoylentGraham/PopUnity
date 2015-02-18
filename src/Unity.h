#pragma once

#include <ofxSoylent.h>
#include <TJob.h>
#include <TChannel.h>
#include "UnityDevice.h"


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
	TCopyTextureCommand() :
		mConvertToFormat	( SoyPixelsFormat::Invalid ),
		mStretch			( false )
	{
	}
	
	inline bool				operator==(const Unity::TTexture& Texture) const	{	return mTexture == Texture;	}
	
public:
	TJobParam				mPixelsParam;
	Unity::TTexture			mTexture;
	SoyPixelsFormat::Type	mConvertToFormat;	//	in case we want to convert the format. Used for kinectdepth -> greyscale (instead of default to RGB)
	bool					mStretch;
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
	
	void			CopyTexture(TJobParam PixelsParam,Unity::TTexture Texture,SoyPixelsFormat::Type ConvertToFormat,bool Stretch);
	void			ProcessCopyTextureQueue();
	
private:
	void			OnJobRecieved(TJobAndChannel& JobAndChannel);	//	special job handling to send back to unity
	
private:
	TLockQueue<std::shared_ptr<TJob>>	mPendingJobs;
	std::mutex						mDebugMessagesLock;
	Array<std::string>				mDebugMessages;	//	gr: might need to be threadsafe
	ofMutexT<Array<TCopyTextureCommand>>	mCopyTextureQueue;
	SoyPixels						mTexturePixelsBuffer;	//	save realloc
};


