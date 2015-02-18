#include "Unity.h"
#include "UnityDevice.h"
#include <PopMain.h>
#include <TProtocolCli.h>
#include "RemoteArray.h"

std::shared_ptr<PopUnity> gApp;

namespace UnityEvent
{
	SoyEvent<int>	mOnPostRender;
	SoyEvent<bool>	mOnStopped;

};

extern "C" void EXPORT_API FlushDebug(Unity::LogCallback LogFunc)
{
	PopUnity::Get().FlushDebugMessages(LogFunc);
}

extern "C" void EXPORT_API OnStopped()
{
	bool Dummy = true;
	UnityEvent::mOnStopped.OnTriggered(Dummy);
	
	//	gr: todo; clear singleton to emulate built app better
}

TPopAppError::Type PopMain(TJobParams& Params)
{
	//	construct early; gr: maybe too early, or is this loaded on first call?
	PopUnity::Get();
	
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
	UnityEvent::mOnPostRender.AddListener( [this](int&) { this->ProcessCopyTextureQueue(); } );
	UnityEvent::mOnStopped.AddListener( [this](bool&) { this->OnStopped(); } );
}

void PopUnity::OnStopped()
{
	//	close all channels
	for ( auto it=mChannels.begin();	it!=mChannels.end();	it++ )
	{
		auto& Channel = *it;
		Channel->Shutdown();
		Channel.reset();
	}
	mChannels.clear();
}

void PopUnity::OnDebug(const std::string& Debug)
{
	std::lock_guard<std::mutex> Lock(mDebugMessagesLock);
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
	std::lock_guard<std::mutex> Lock(mDebugMessagesLock);
	if ( !LogFunc )
		mDebugMessages.Clear();
	
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
	
	return mPendingJobs.Pop();
}

void PopUnity::PushJob(TJobAndChannel& JobAndChannel)
{
	std::shared_ptr<TJob> Job( new TJob(JobAndChannel.GetJob() ) );
	mPendingJobs.Push( Job );
	
}


void PopUnity::ProcessCopyTextureQueue()
{
	while ( !mCopyTextureQueue.IsEmpty() )
	{
		mCopyTextureQueue.lock();
		TCopyTextureCommand Copy = mCopyTextureQueue.PopBack();
		mCopyTextureQueue.unlock();
		
		if ( !Unity::gDevice )
			continue;
		
		if ( !Copy.mTexture.IsValid() )
		{
			std::Debug << "CopyTexture failed as target texture is invalid" << std::endl;
			continue;
		}

		SoyData_Impl<SoyPixels> Pixels(mTexturePixelsBuffer);
		if ( !Copy.mPixelsParam.Decode( Pixels ) )
			continue;
		
		//	want to reformat
		if ( Copy.mConvertToFormat != SoyPixelsFormat::Invalid && Pixels.mValue.GetFormat() != Copy.mConvertToFormat )
		{
			if ( !Pixels.mValue.SetFormat( Copy.mConvertToFormat ) )
			{
				std::Debug << "CopyTexture() Failed to convert pixels from " << Pixels.GetFormat() << " to " << Copy.mConvertToFormat << std::endl;
			}
		}
		
		Unity::gDevice->CopyTexture( Copy.mTexture, Pixels.mValue, true, Copy.mStretch );
	}
}


void PopUnity::CopyTexture(TJobParam PixelsParam,Unity::TTexture Texture,SoyPixelsFormat::Type ConvertToFormat,bool Stretch)
{
	mCopyTextureQueue.lock();
	
	//	if we have this texture already in the queue, replace it
	TCopyTextureCommand* pCopyCommand = nullptr;
	pCopyCommand = mCopyTextureQueue.Find( Texture );
	
	//	doesn't already exist, alloc new entry
	if ( !pCopyCommand )
	{
		pCopyCommand = &mCopyTextureQueue.PushBack();
	}

	auto& Copy = *pCopyCommand;
	Copy.mPixelsParam = PixelsParam;
	Copy.mTexture = Texture;
	Copy.mConvertToFormat = ConvertToFormat;
	Copy.mStretch = Stretch;
	mCopyTextureQueue.unlock();
	
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
	{
		std::Debug << "Failed to send command(" << Command << ") to non-existant channel " << Channel << std::endl;
		return false;
	}
	
	TJob Job;
	if ( !TProtocolCli::DecodeHeader( Job, Command ) )
	{
		std::Debug << "Failed to decode command for job: " << Command << std::endl;
		return false;
	}
	
	//std::Debug << "Sending job: " << Job.mParams << std::endl;

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



extern "C" int EXPORT_API GetJobParam_int(TJobInterface* JobInterface,const char* Param,int DefaultValue)
{
	auto& Job = *JobInterface->mTJob;
	auto Value = Job.mParams.GetParamAsWithDefault<int>( Param, DefaultValue );
	return Value;
}


extern "C" float EXPORT_API GetJobParam_float(TJobInterface* JobInterface,const char* Param,float DefaultValue)
{
	auto& Job = *JobInterface->mTJob;
	auto Value = Job.mParams.GetParamAsWithDefault<float>( Param, DefaultValue );
	return Value;
}


extern "C" const char* EXPORT_API GetJobParam_string(TJobInterface* JobInterface,const char* Param,const char* DefaultValue)
{
	//	gr: to avoid memory management, we leave the variable in a static
	//		in theory, this is only called whilst the job interface exists
	//		(so we could put a buffer there) but the result will be copied
	//		to c# memory so we don't need to worry about this being changed
	//		whilst in use
	static std::string Value;

	//	re-use this static
	auto& DefaultValueString = Value;
	DefaultValueString = DefaultValue;
	
	auto& Job = *JobInterface->mTJob;
	Value = Job.mParams.GetParamAsWithDefault<std::string>( Param, DefaultValueString );
	
	return Value.c_str();
}


extern "C" bool EXPORT_API GetJobParam_texture(TJobInterface* JobInterface,const char* ParamName,int Texture,SoyPixelsFormat::Type Format,bool Stretch)
{
	auto& Job = *JobInterface->mTJob;

	auto Param = Job.mParams.GetParam( ParamName );
	if ( !Param.IsValid() )
	{
		std::Debug << "No such param " << ParamName << std::endl;
		return false;
	}
	
	//	don't extract now and extract during upload in case it's a memfile and we can get the very latest image
	auto& App = PopUnity::Get();
	App.CopyTexture( Param, Unity::TTexture(Texture), Format, Stretch );
	
	return true;
}


extern "C" bool EXPORT_API GetJobParam_PixelsWidthHeight(TJobInterface* JobInterface,const char* ParamName,int* Width,int* Height)
{
	auto& Job = *JobInterface->mTJob;
	
	//	pull image
	SoyPixels Pixels;
	if ( !Job.mParams.GetParamAs( ParamName, Pixels ) )
	{
		std::Debug << "Failed to extract pixels from param " << ParamName << std::endl;
		return false;
	}

	//	get image dimensions
	*Width = Pixels.GetWidth();
	*Height = Pixels.GetHeight();
	
	return true;
}

#include <TFeatureBinRing.h>


class TFeatureMatchCSharp
{
public:
	uint32	x;
	uint32	y;
	float	score;
};

template<typename ELEMENTTYPE>
bool TryExtractArray(const TJobParams& Params,const std::string& ParamName,const std::string& TypeName,void* Buffer,int& BufferSize)
{
	//	typename doesn't come from Soy::GetTypeName so we can't use is_sametypename
	if ( !Soy::StringMatches( Soy::GetTypeName<ELEMENTTYPE>(), TypeName, false ) )
		return false;
	
	Array<ELEMENTTYPE> ParamArray;
	if ( !Params.GetParamAs( ParamName, ParamArray ) )
		return false;

	//	save real size
	int RealSize = ParamArray.GetSize();
	
	//	cap data so we don't overwrite mem
	//if ( ParamArray.GetSize() > BufferSize )
	//	ParamArray.SetSize( BufferSize );
	//	memcpy( Buffer, ParamArray.GetArray(), ParamArray.GetDataSize() );

	//	last bit crashes. wrong ptrs?
	TFeatureMatchCSharp* BufferSmall = reinterpret_cast<TFeatureMatchCSharp*>(Buffer);
	auto SmallFeatures = GetRemoteArray( BufferSmall, BufferSize );
	for ( int i=0;	i<std::min(BufferSize,ParamArray.GetSize());	i++ )
	{
		auto& Fm = ParamArray[i];
		SmallFeatures[i].x = Fm.mCoord.x;
		SmallFeatures[i].y = Fm.mCoord.y;
		SmallFeatures[i].score = Fm.mScore;
	}

	//	return real size
	BufferSize = RealSize;
	return true;
}

extern "C" int EXPORT_API GetJobParam_Array(TJobInterface* JobInterface,const char* ParamName,const char* ElementTypeName,void* Array,int ArraySize)
{
	auto& Job = *JobInterface->mTJob;

	//	find param
	auto Param = Job.mParams.GetParam( ParamName );
	if ( !Param.IsValid() )
		return -1;
	
	//	decode to our format, then encode to binary so we can memcpy.
	//	gr: to skip a step, check if it's already binary/ourformat. Any other format we have to decode (memfile, gzip etc)
	std::stringstream ArrayElementFormat;
	ArrayElementFormat << "array<" << ElementTypeName << ", prcore::heap>";
	TJobFormat Format( ArrayElementFormat.str() );
	if ( !Param.GetFormat().HasContainer( ArrayElementFormat.str() ) )
	{
		std::Debug << "Param " << ParamName << " (" << Param.GetFormat() << ") does not contain " << ArrayElementFormat.str() << std::endl;
		return -1;
	}

	if ( TryExtractArray<TFeatureMatch>( Job.mParams, ParamName, ElementTypeName, Array, ArraySize ) )
		return ArraySize;
	
	std::Debug << "don't know how to handle type " << ElementTypeName << std::endl;
	return -1;
}

