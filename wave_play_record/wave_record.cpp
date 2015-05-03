#include <stdio.h>
#include "portaudio.h"
#include "wave_file.h"

#define FRAMES_PER_BUF   2048

static int callBack(const void *inputBuf, void *outputBuf, 
	unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, 
	PaStreamCallbackFlags flag, void *userData)
{
	WaveWrite(inputBuf, 2, framesPerBuffer);
	return 0;
}

int main(void)
{
	// create wave file
	WaveCreate("dst.wav");

	PaStreamParameters inputParameters;
	PaStream *stream;
	PaError err;

	//// 初始化
	err = Pa_Initialize();
	if ( err != paNoError ) goto error;

	inputParameters.device = Pa_GetDefaultOutputDevice();	// default output device
	if (inputParameters.device == paNoDevice)
	{
		fprintf(stderr, "Error: No default output device.\n");
		goto error;
	}

	// 参数：通道(1/2)，位数(16/32/int/float)
	inputParameters.channelCount = 1;
	inputParameters.sampleFormat = paInt16;
	inputParameters.suggestedLatency = 0.050;	// Pa_GetDeviceInfo( inputParameters.device )->defaultLowOutputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	//// 打开
	err = Pa_OpenStream(
		&stream,
		&inputParameters,
		NULL,		// no output
		44100,
		FRAMES_PER_BUF,
		paClipOff,	// we won't output out of range samples so don't bother clipping them
		callBack,	// no callback, use blocking API
		NULL		// no callback, so no callback userData
		);
	if ( err != paNoError ) goto error;

	//// 启动
	err = Pa_StartStream( stream );
	if ( err != paNoError ) goto error;

	//// 等待录音
	printf("\n=== Start recording... ===\n");
	for(int i = 0; i < 10; ++i) {
		printf("second %d\n", i);
		Pa_Sleep(1000);
	}
	printf("\n=== Stop recording. ===\n");

	//// 停止
	err = Pa_StopStream( stream );
	if ( err != paNoError ) goto error;

	err = Pa_CloseStream( stream );
	if ( err != paNoError ) goto error;

	//// 结束
	Pa_Terminate();
	printf("Record finished.\n");

	// 写入文件头并保存wav文件
	WaveWriteHeader(0);

	return 0;

//// Error
	error:
	fprintf( stderr, "An error occured while using the portaudio stream\n" );
	fprintf( stderr, "Error number: %d\n", err );
	fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
	// Print more information about the error.
	if ( err == paUnanticipatedHostError )
	{
		const PaHostErrorInfo *hostErrorInfo = Pa_GetLastHostErrorInfo();
		fprintf( stderr, "Host API error = #%ld, hostApiType = %d\n", hostErrorInfo->errorCode, hostErrorInfo->hostApiType );
		fprintf( stderr, "Host API error = %s\n", hostErrorInfo->errorText );
	}
	Pa_Terminate();
	return err;
}
