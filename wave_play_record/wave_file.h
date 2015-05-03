#include <stdio.h>

// 64位Ubuntu g++
typedef unsigned char	uchar;
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef char			sint8;
typedef short			sint16;
typedef int				sint32;
typedef float			fp32;
typedef double			fp64;

// 32位Ubuntu g++
// typedef unsigned char	uchar;
// typedef unsigned char	uint8;
// typedef unsigned short	uint16;
// typedef unsigned long	uint32;
// typedef char				sint8;
// typedef short			sint16;
// typedef long				sint32;
// typedef float			fp32;
// typedef double			fp64;

//wave文件头
// 12+24=36 Bytes(0~35 / 00H~23H)
typedef struct WaveHeader
{
	// 1.RIFF Chunk 12 Bytes
	uint8 riff[4];			//资源交换文件标志
	uint32 size;			//从下个地址开始到文件结尾的字节数
	uint8 wave_flag[4];		//wave文件标识
	// 2.Format Chunk 24(26) Bytes
	uint8 fmt[4];			//波形格式标识
	uint32 fmt_len;			//过滤字节(一般为00000010H)
	uint16 tag;				//格式种类，值为1时，表示PCM线性编码
	uint16 channels;		//通道数，单声道为1，双声道为2
	uint32 samp_freq;		//采样频率
	uint32 byte_rate;		//数据传输率 (每秒字节＝采样频率×每个样本字节数)
	uint16 block_align;		//块对齐字节数 = channles * bit_samp / 8
	uint16 bit_samp;		//bits per sample (又称量化位数)
} wave_header_t;

// 3.Fact Chunk 12 Bytes (Optional)可能没有

// wave文件
typedef struct WaveStruct
{
	FILE *fp;				//file pointer
	wave_header_t header;	//header
	// 4.Data Chunk 8 Bytes + Datas(From 24H)
	uint8 data_flag[4];		//数据标识符"data"
	uint32 length;			//采样数据总数0x0010AA00
	uint32 *pData;			//data
} wave_t;
wave_t wave;

// 相同返回true，不同返回false
bool StrCmp(uint8 s[], const char *d, int len)
{
	for(int i = 0; i < len; ++i) {
		if(s[i] != d[i])
			return false;
	}
	return true;
}

// 打开文件 open *.wav file
void WaveOpen(const char *file)
{
	uchar temp = 0;
	uint8 read_bytes = 0;
	const char *channel_mappings[] = {NULL, "mono", "stereo"};
	uint32 total_time = 0;
	struct PlayTime		//播放时间
	{
		uint8 hour;
		uint8 minute;
		uint8 second;
	} play_time;

	// 打开文件
	if(NULL == (wave.fp = fopen(file, "rb"))) {
		printf("file %s open failure!\n", file);
		return;
	}

	// 读取文件头
	fread(&wave.header, sizeof(wave.header), 1, wave.fp);

	// jump to "data" for reading data
	do {
		fread(&temp, sizeof(uchar), 1, wave.fp);
	} while('d' != temp);

	wave.data_flag[0] = temp;
	// data chunk
	if(3 != fread(&wave.data_flag[1], sizeof(uint8), 3, wave.fp)) {
		printf("read header data error!\n");
		return;
	}
	// data length
	if(1 != fread(&wave.length, sizeof(uint32), 1, wave.fp)) {
		printf("read length error!\n");
	}

	// jduge data chunk flag
	if(false == StrCmp(wave.data_flag, "data", 4)) {
		printf("error : cannot read data!\n");
		return;
	} else {
		printf("data read success!\n");
	}

	total_time = wave.length / wave.header.byte_rate;
	play_time.hour = (uint8)(total_time / 3600);
	play_time.minute = (uint8)((total_time / 60) % 60);
	play_time.second = (uint8)(total_time % 60);
	// printf file header information
	printf("%s %dHz %dbit, DataLen: %d, Rate: %d, Length: %02d:%02d:%02d\n",
			channel_mappings[wave.header.channels],		//声道
			wave.header.samp_freq,						//采样频率
			wave.header.bit_samp,						//每个采样点的量化位数
			wave.length,
			wave.header.byte_rate,
			play_time.hour, play_time.minute, play_time.second
			);

	//fclose(wave.fp);									// close wave file
}

// 读取文件，参数：缓冲区，每个数据字节长度，数据总数
long WaveRead(void *buffer, int size, long frames)
{
	long p = 0;
	if(size == 2) {
		uint16 *buf = (uint16 *)buffer;
		p = fread(buf, sizeof(uint16), frames, wave.fp);
		if(p == 0) {
			return -1; // error
		} else {
			for(; p < frames; ++p) {
				 *(buf + p) = 0;
			}
			return frames;
		}
	} else if(size == 4) {
		uint32 *buf = (uint32 *)buffer;
		p = fread(buf, sizeof(uint32), frames, wave.fp);
		if(p == 0) {
			return -1; // error
		} else {
			for(; p < frames; ++p) {
				 *(buf + p) = 0;
			}
			return frames;
		}
	}
}

// 记录写入数据长度
uint32 wave_record_size;

// 创建文件 create wave file and seek to data chunk
void WaveCreate(const char *file)
{
	if(NULL == (wave.fp = fopen(file, "wb"))) {
		printf("file %s create failure!\n", file);
		return;
	}
	fseek(wave.fp, sizeof(wave.header) + 8, SEEK_SET); //"data"+size / (4+4=8)
	wave_record_size = 0;
}

// 写入数据
//  size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
void WaveWrite(const void *buf, int size, int count)
{
	wave_record_size += size*fwrite(buf, size, count, wave.fp);
}

// 写入文件头并保存文件
void WaveWriteHeader(uint32 size)
{
	if(size <= 0) {
		size = wave_record_size;
	}
	fseek(wave.fp, 0, SEEK_SET);
	wave.header.riff[0] = 'R';
	wave.header.riff[1] = 'I';
	wave.header.riff[2] = 'F';
	wave.header.riff[3] = 'F';
	wave.header.size = 0x24 + size;
	wave.header.wave_flag[0] = 'W';
	wave.header.wave_flag[1] = 'A';
	wave.header.wave_flag[2] = 'V';
	wave.header.wave_flag[3] = 'E';
	wave.header.fmt[0] = 'f';
	wave.header.fmt[1] = 'm';
	wave.header.fmt[2] = 't';
	wave.header.fmt[3] = ' ';
	wave.header.fmt_len = 0x10;
	wave.header.tag = 1;
	wave.header.channels = 1;
	wave.header.samp_freq = 44100;
	wave.header.byte_rate = wave.header.samp_freq*2;
	wave.header.block_align = 0x02;
	wave.header.bit_samp = 16;
	if(1 != fwrite((void *) (&wave.header), sizeof(wave.header), 1, wave.fp)) {
		printf("write file header failure!\n");
		return;
	}
	if(4 != fwrite("data", 1, 4, wave.fp)) {//"data"
		printf("write file header \"data\" failure!\n");
		return;
	}
	if(1 != fwrite(&size, 4, 1, wave.fp)) {//size
		printf("write file header size failure!\n");
		return;
	}
	printf("write file header success!\n");
	fclose(wave.fp);
	printf("wave file saved!\n");
}

/*
	// read heade information
	if(4 != fread(wave.header.riff, sizeof(uint8), 4, wave.fp))		// RIFF chunk
	{
		printf("read riff error!\n");
		return;
	}
	if(1 != fread(&wave.header.size, sizeof(uint32), 1, wave.fp))		// SIZE : from here to file end
	{
		printf("read size error!\n");
		return;
	}
	if(4 != fread(wave.header.wave_flag, sizeof(uint8), 4, wave.fp))	// wave file flag
	{
		printf("read wave_flag error!\n");
		return;
	}
	if(4 != fread(wave.header.fmt, sizeof(uint8), 4, wave.fp))			// fmt chunk
	{
		printf("read fmt error!\n");
		return;
	}
	if(1 != fread(&wave.header.fmt_len, sizeof(uint32), 1, wave.fp))	// fmt length
	{
		printf("read fmt_len error!\n");
		return;
	}
	if(1 != fread(&wave.header.tag, sizeof(uint16), 1, wave.fp))		// tag : PCM or not
	{
		printf("read tag error!\n");
		return;
	}
	if(1 != fread(&wave.header.channels, sizeof(uint16), 1, wave.fp))	// channels
	{
		printf("read channels error!\n");
		return;
	}
	if(1 != fread(&wave.header.samp_freq, sizeof(uint32), 1, wave.fp))	// samp_freq
	{
		printf("read samp_freq error!\n");
		return;
	}
	if(1 != fread(&wave.header.byte_rate, sizeof(uint32), 1, wave.fp))	// byte_rate : decode how many bytes per second
	{																	// byte_rate = samp_freq * bit_samp
		printf("read byte_rate error!\n");
		return;
	}
	if(1 != fread(&wave.header.block_align, sizeof(uint16), 1, wave.fp))	// quantize bytes for per samp point
	{
		printf("read byte_samp error!\n");
		return;
	}
	if(1 != fread(&wave.header.bit_samp, sizeof(uint16), 1, wave.fp))		// quantize bits for per samp point
	{																		// bit_samp = byte_samp * 8
		printf("read bit_samp error!\n");
		return;
	}
*/
