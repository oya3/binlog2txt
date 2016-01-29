// binlog.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <time.h>
#include <windows.h>

typedef unsigned int uint32;

class Locker
{
private:
    CRITICAL_SECTION &m_locker;

public:
    Locker(CRITICAL_SECTION &locker) : m_locker(locker)
	{
		EnterCriticalSection(&m_locker);
	}
    ~Locker()
	{
		LeaveCriticalSection(&m_locker);
	}

private:
    Locker(); // 禁止
    Locker& operator=(const Locker&);
};

class Binlog
{
private:
	typedef struct {
		uint32 time;
		uint32 string;
		uint32 parameter1;
		uint32 parameter2;
	}LOG;

	bool m_initialized;
	bool m_looped; // ループ状態
	uint32 m_count; // 件数
	uint32 m_index; // 開始
	uint32 m_magic_keyword;
	LOG* m_plog;
	CRITICAL_SECTION m_memory;

private:
	Binlog() : m_initialized(false),
	           m_index(0),
			   m_looped(false),
		       m_plog(NULL)
	{
		InitializeCriticalSection(&m_memory);
	}

	~Binlog()
	{
		if (NULL != m_plog){
			delete m_plog;
			m_plog = NULL;
		}
	}

public:
	static Binlog& Binlog::getInstance()
	{
		static Binlog s_instance; // 何か問題があった気がするけど。。。
		return s_instance;
	}

	void initialize(int count)
	{
		if(m_initialized){
			return; // 初期化済み
		}
		m_count = count;
		m_plog = new LOG[m_count];
		m_index = 0;
		m_looped = false;
		m_magic_keyword = (uint32)"@DOUBLE_MCTWIST@";
		m_initialized = true;
	}

	void puts(char *pstring, uint32 parameter1, uint32 parameter2)
	{
		if(!m_initialized){
			return; // 未初期化
		}

		Locker lock(m_memory); // ロック

		LOG& log = m_plog[m_index];
		m_index++;
		m_index %= m_count;
		if( m_index == 0 ){
			m_looped = true; // ループ済
		}

		// 現在時刻取得
		time_t timer;
		time(&timer);

		log.time = (uint32)timer;
		log.string = (uint32)pstring;
		log.parameter1 = (uint32)parameter1;
		log.parameter2 = (uint32)parameter2;
	}

	void write(char* pfile)
	{
		if(!m_initialized){
			return; // 未初期化
		}

		Locker lock(m_memory);// ロック開始

		std::ofstream fout;
		fout.open(pfile, std::ios::out|std::ios::binary|std::ios::trunc);

		fout.write( (char*)&m_count,sizeof(m_count)); // 件数

		// uint32 looped = m_looped ? 1: 0;
		if(m_looped){
			// ループ済
			fout.write( (char*)&m_index,sizeof(uint32)); // 開始位置
			fout.write( (char*)&m_count,sizeof(uint32)); // 終了位置(m_countと同値の場合はループ済を示す)
		}
		else{
			// ループ未
			uint32 start_index = 0;
			fout.write( (char*)&start_index,sizeof(uint32)); // 開始位置
			fout.write( (char*)&m_index,sizeof(uint32)); // 終了位置
		}

		fout.write( (char*)&m_magic_keyword,sizeof(m_magic_keyword)); // マジックキーワード
		for(int index=0; index<(int)m_count; ++index){
			LOG& log = m_plog[index];
			fout.write( (char*)&log.time,sizeof(log.time));
			fout.write( (char*)&log.string,sizeof(log.string));
			fout.write( (char*)&log.parameter1,sizeof(log.parameter1));
			fout.write( (char*)&log.parameter2,sizeof(log.parameter2));
		}
		fout.close();
	}

};

#define BINLOG_INITIALIZE(c) Binlog::getInstance().initialize(c);
#define BINLOG(s,p1,p2) Binlog::getInstance().puts(s,(uint32)p1,(uint32)p2)
#define BINLOG_WRITE(f) Binlog::getInstance().write(f)

int _tmain(int argc, _TCHAR* argv[])
{
	BINLOG_INITIALIZE(10); // 初期化 10件のログ
	for(int index=0; index<21; ++index){
		BINLOG("表示能力①②③", index, index);
	}
	BINLOG("abc123", 0, 0);
	BINLOG_WRITE("test.binlog"); // binlog書き出し

	return 0;
}

