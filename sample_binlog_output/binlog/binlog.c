// Cソースサンプル
#include <stdio>
#include "binlog.h"

// 使い方
// BINLOG_INITIALIZE(10);
// BINLOG("表示能力①②③", 0,0);
// BINLOG("abc123", 0, 0);
// BINLOG_WRITE("test.binlog");

static Binlog* s_pinstance = NULL; /* binlog インスタンス保持用 */

static void __initialize(Binlog* self, int count);
static void __puts(Binlog* self, char *pstring, uint32_t parameter1, uint32_t parameter2);
static void __write(Binlog* self, char* pfile);

/*
 * @brief Binlog インスタンス取得
 * @details
 * @param なし
 * @retval Binlog* Binlogインスタンス
 * @attention なし
 */
Binlog* Binlog_getInstance();
{
	if(NULL == s_pinstance){
		s_pinstance = malloc(sizeof(Binlog)); /* 出力先バッファーを固定してももちろんいい */
		if(NULL == s_pinstance){
			PANIC("get_BinlogInstance(): bad allocation.")
		}
		s_pinstance->m_initialized = false;
		s_pinstance->initialize = __initialize;
		s_pinstance->puts = __puts;
		s_pinstance->write = __write;
	}
	return s_pinstance;
}

/*
 * @brief Binlog インスタンス破棄
 * @details
 * @param なし
 * @return なし
 * @attention なし
 */
void Binlog_deleteInstance()
{
	Binlog* self = Binlog_getInstance();
	if(NULL == self){
		return;
	}
	SAFE_FREE(self->m_plog);
	SAFE_FREE(self);
}

/*
 * @brief Binlog 初期化
 * @details
 * @param[in] self  Binlogインスタンス
 * @param[in] count ログ件数
 * @return なし
 * @attention なし
 */
static void __initialize(Binlog* self, int count)
{
	if(self->m_initialized){
		return; // 初期化済み
	}
	self->m_count = count;
	self->m_plog = new LOG[self->m_count];
	self->m_index = 0;
	self->m_looped = false;
	self->m_magic_keyword = (uint32_t)"@DOUBLE_MCTWIST@";
	self->m_initialized = true;
}

/*
 * @brief Binlog 出力
 * @details
 * @param[in] self       Binlogインスタンス
 * @param[in] pstring    タイトル
 * @param[in] parameter1 パラメータ１
 * @param[in] parameter2 パラメータ２
 * @return なし
 * @attention なし
 */
static void __puts(Binlog* self, char *pstring, uint32_t parameter1, uint32_t parameter2)
{
	if(!self->m_initialized){
		return; // 未初期化
	}
	
	/* Locker lock(self->m_memory); // ロック */
	
	LOG& log = self->m_plog[self->m_index];
	self->m_index++;
	self->m_index %= self->m_count;
	if( self->m_index == 0 ){
		self->m_looped = true; // ループ済
	}
	
	// 現在時刻取得
	time_t timer;
	time(&timer);
	
	log.time = (uint32_t)timer;
	log.string = (uint32_t)pstring;
	log.parameter1 = (uint32_t)parameter1;
	log.parameter2 = (uint32_t)parameter2;
}

/*
 * @brief Binlog 書き出し
 * @details
 * @param[in] self  Binlogインスタンス
 * @param[in] pfile ファイル名
 * @return なし
 * @attention なし
 */
static void __write(Binlog* self, char* pfile)
{
	if(!self->m_initialized){
		return; // 未初期化
	}
	
	/* Locker lock(self->m_memory);// ロック開始 */

	FILE* fp = fopen(pfile,"wb");
	if(NULL == fp){
		return;
	}
	fwrite((char*)&self->m_count,sizeof(self->m_count),1,fp); /* 件数 */
	
	if(self->m_looped){
		/* ループ済 */
		fwrite((char*)&self->m_index,sizeof(uint32_t),1,fp); /* 開始位置 */
		fwrite((char*)&self->m_count,sizeof(uint32_t),1,fp); /* 終了位置(self->m_countと同値の場合はループ済を示す) */
	}
	else{
		/* ループ未 */
		uint32_t start_index = 0;
		fwrite((char*)&start_index,sizeof(uint32_t),1,fp); /* 開始位置 */
		fwrite((char*)&self->m_index,sizeof(uint32_t),1,fp); /* 終了位置 */
	}
	
	fwrite((char*)&self->m_magic_keyword,sizeof(self->m_magic_keyword),1,fp); /* マジックキーワード */
	for(int index=0; index<(int)self->m_count; ++index){
		LOG& log = self->m_plog[index];
		fwrite((char*)&log.time,sizeof(log.time),1,fp);
		fwrite((char*)&log.string,sizeof(log.string),1,fp);
		fwrite((char*)&log.parameter1,sizeof(log.parameter1),1,fp);
		fwrite((char*)&log.parameter2,sizeof(log.parameter2),1,fp);
	}
	fclose(fp);
}

