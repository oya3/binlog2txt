// C�\�[�X�T���v��
#include <stdio>
#include "binlog.h"

// �g����
// BINLOG_INITIALIZE(10);
// BINLOG("�\���\�͇@�A�B", 0,0);
// BINLOG("abc123", 0, 0);
// BINLOG_WRITE("test.binlog");

static Binlog* s_pinstance = NULL; /* binlog �C���X�^���X�ێ��p */

static void __initialize(Binlog* self, int count);
static void __puts(Binlog* self, char *pstring, uint32_t parameter1, uint32_t parameter2);
static void __write(Binlog* self, char* pfile);

/*
 * @brief Binlog �C���X�^���X�擾
 * @details
 * @param �Ȃ�
 * @retval Binlog* Binlog�C���X�^���X
 * @attention �Ȃ�
 */
Binlog* Binlog_getInstance();
{
	if(NULL == s_pinstance){
		s_pinstance = malloc(sizeof(Binlog)); /* �o�͐�o�b�t�@�[���Œ肵�Ă�������񂢂� */
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
 * @brief Binlog �C���X�^���X�j��
 * @details
 * @param �Ȃ�
 * @return �Ȃ�
 * @attention �Ȃ�
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
 * @brief Binlog ������
 * @details
 * @param[in] self  Binlog�C���X�^���X
 * @param[in] count ���O����
 * @return �Ȃ�
 * @attention �Ȃ�
 */
static void __initialize(Binlog* self, int count)
{
	if(self->m_initialized){
		return; // �������ς�
	}
	self->m_count = count;
	self->m_plog = new LOG[self->m_count];
	self->m_index = 0;
	self->m_looped = false;
	self->m_magic_keyword = (uint32_t)"@DOUBLE_MCTWIST@";
	self->m_initialized = true;
}

/*
 * @brief Binlog �o��
 * @details
 * @param[in] self       Binlog�C���X�^���X
 * @param[in] pstring    �^�C�g��
 * @param[in] parameter1 �p�����[�^�P
 * @param[in] parameter2 �p�����[�^�Q
 * @return �Ȃ�
 * @attention �Ȃ�
 */
static void __puts(Binlog* self, char *pstring, uint32_t parameter1, uint32_t parameter2)
{
	if(!self->m_initialized){
		return; // ��������
	}
	
	/* Locker lock(self->m_memory); // ���b�N */
	
	LOG& log = self->m_plog[self->m_index];
	self->m_index++;
	self->m_index %= self->m_count;
	if( self->m_index == 0 ){
		self->m_looped = true; // ���[�v��
	}
	
	// ���ݎ����擾
	time_t timer;
	time(&timer);
	
	log.time = (uint32_t)timer;
	log.string = (uint32_t)pstring;
	log.parameter1 = (uint32_t)parameter1;
	log.parameter2 = (uint32_t)parameter2;
}

/*
 * @brief Binlog �����o��
 * @details
 * @param[in] self  Binlog�C���X�^���X
 * @param[in] pfile �t�@�C����
 * @return �Ȃ�
 * @attention �Ȃ�
 */
static void __write(Binlog* self, char* pfile)
{
	if(!self->m_initialized){
		return; // ��������
	}
	
	/* Locker lock(self->m_memory);// ���b�N�J�n */

	FILE* fp = fopen(pfile,"wb");
	if(NULL == fp){
		return;
	}
	fwrite((char*)&self->m_count,sizeof(self->m_count),1,fp); /* ���� */
	
	if(self->m_looped){
		/* ���[�v�� */
		fwrite((char*)&self->m_index,sizeof(uint32_t),1,fp); /* �J�n�ʒu */
		fwrite((char*)&self->m_count,sizeof(uint32_t),1,fp); /* �I���ʒu(self->m_count�Ɠ��l�̏ꍇ�̓��[�v�ς�����) */
	}
	else{
		/* ���[�v�� */
		uint32_t start_index = 0;
		fwrite((char*)&start_index,sizeof(uint32_t),1,fp); /* �J�n�ʒu */
		fwrite((char*)&self->m_index,sizeof(uint32_t),1,fp); /* �I���ʒu */
	}
	
	fwrite((char*)&self->m_magic_keyword,sizeof(self->m_magic_keyword),1,fp); /* �}�W�b�N�L�[���[�h */
	for(int index=0; index<(int)self->m_count; ++index){
		LOG& log = self->m_plog[index];
		fwrite((char*)&log.time,sizeof(log.time),1,fp);
		fwrite((char*)&log.string,sizeof(log.string),1,fp);
		fwrite((char*)&log.parameter1,sizeof(log.parameter1),1,fp);
		fwrite((char*)&log.parameter2,sizeof(log.parameter2),1,fp);
	}
	fclose(fp);
}

