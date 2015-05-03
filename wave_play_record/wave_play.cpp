#include <stdio.h>
#include "portaudio.h"
#include "wave_file.h"

#define FRAMES_PER_BUF   1024

static int callBack(const void *inputBuf, void *outputBuf, 
	unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, 
	PaStreamCallbackFlags flag, void *userData)
{
	WaveRead(outputBuf, 2, framesPerBuffer);
	// uint16 *buffer = (uint16 *)outputBuf;
	// fread(buffer, sizeof(uint16), framesPerBuffer, wave.fp);
	return 0;
}

int main(void)
{
	WaveOpen("src.wav");

	//// 打印信息
	printf("SampleRate = %d, frames = %d, channels = %d, bits = %d\n",
		wave.header.samp_freq,
		FRAMES_PER_BUF,
		wave.header.channels,
		wave.header.bit_samp
		);

	PaStreamParameters outputParameters;
	PaStream *stream;
	PaError err;

	//// 初始化
	err = Pa_Initialize();
	if ( err != paNoError ) goto error;

	outputParameters.device = Pa_GetDefaultOutputDevice();	// default output device
	if (outputParameters.device == paNoDevice) {
		fprintf(stderr, "Error: No default output device.\n");
		goto error;
	}

	// 参数：通道(1/2)，位数(16/32,int/float)
	outputParameters.channelCount = wave.header.channels;
	if(wave.header.bit_samp == 16)
		outputParameters.sampleFormat = paInt16;
	else if(wave.header.bit_samp == 32)
		outputParameters.sampleFormat = paInt32;

	outputParameters.suggestedLatency = 0.050;	// Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	//// 打开
	err = Pa_OpenStream(
		&stream,
		NULL,		// no input
		&outputParameters,
		wave.header.samp_freq,
		FRAMES_PER_BUF,
		paClipOff,	// we won't output out of range samples so don't bother clipping them
		callBack,	// callback, not using blocking API
		NULL		// no callback, so no callback userData
		);
	if ( err != paNoError ) goto error;

	//// 启动
	err = Pa_StartStream( stream );
	if ( err != paNoError ) goto error;

	//// 等待播放完成
	Pa_Sleep(13 * 1000);

	//// 停止
	err = Pa_StopStream( stream );
	if ( err != paNoError ) goto error;

	err = Pa_CloseStream( stream );
	if ( err != paNoError ) goto error;

	//// 结束
	Pa_Terminate();
	printf("Play finished.\n");
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
