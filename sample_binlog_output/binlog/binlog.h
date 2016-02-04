#pragma once

typedef struct
{
	typedef struct {
		uint32_t time;
		uint32_t string;
		uint32_t parameter1;
		uint32_t parameter2;
	}LOG;

	bool m_initialized;
	bool m_looped; // ループ状態
	uint32_t m_count; // 件数
	uint32_t m_index; // 開始
	uint32_t m_magic_keyword;
	LOG* m_plog;
	
	void (*initialize)(int count);
	void (*puts)(char *pstring, uint32_t parameter1, uint32_t parameter2);
	void (*write)(char* pfile);
}Binlog;

Binlog* Binlog_getInstance();
void Binlog_deleteInstance();

#define BINLOG_INITIALIZE(c) Binlog_getInstance()->initialize(Binlog_getInstance(), c);
#define BINLOG(s,p1,p2) Binlog_getInstance()->puts(Binlog_getInstance(), s,(uint32_t)p1,(uint32_t)p2)
#define BINLOG_WRITE(f) Binlog_getInstance()->write(Binlog_getInstance(), f)

#define SAFE_FREE(x) \
	if(NULL != x) {	 \
		free(x);		 \
		x = NULL;		 \
	}
