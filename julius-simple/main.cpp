// http://d.hatena.ne.jp/kazekyo/20100922/1285161893
#include <julius/juliuslib.h>
#include <stdio.h>

#if 0
#include <iconv.h>

#define STR_OUT 1000

class charconv
{
private:
    iconv_t ic;
public:
    charconv();
    void convert(char *str_in, char *str_out, size_t str_out_size);
    ~charconv();
};

charconv::charconv()
{
    ic = iconv_open("UTF-8", "EUC-JP");
}
charconv::~charconv()
{
    iconv_close(ic);
}
void charconv::convert(char *str_in, char *str_out, size_t str_out_size)
{
    char *ptr_in  = str_in;
    char *ptr_out = str_out;
    size_t inbufsz = strlen(str_in)+1;
    size_t outbufsz = str_out_size-1;
    iconv(ic, &ptr_in, &inbufsz, &ptr_out, &outbufsz);
}
#endif

// �����悤�ɂȂ�����Ăяo�����R�[���o�b�N
static void status_recready(Recog *recog, void *dummy)
{
    if (recog->jconf->input.speech_input == SP_MIC || recog->jconf->input.speech_input == SP_NETAUDIO) {
        fprintf(stderr, "<<< please speak >>>\n");
    }
}


// �ŏI�I�ȔF�����ʂ��A�E�g�v�b�g����R�[���o�b�N
static void output_result(Recog *recog, void *dummy)
{
    WORD_INFO *winfo;
    WORD_ID *seq;
    int seqnum;
    int n,i;
    Sentence *s;
    RecogProcess *r;
    //char    str_out[STR_OUT];
    //charconv *cc = new charconv;

    for(r=recog->process_list;r;r=r->next) {

        // �����������ĂȂ��Ƃ��͔�΂��i�ǂ�Ȏ����͒m��Ȃ����ǁj
        if (! r->live) continue;

        // �������ʂ������Ȃ�������΂��i�ǂ�Ȏ�������j
        if (r->result.status < 0) continue;

        // �S�ẴZ���e���X���A�E�g�v�b�g����
        winfo = r->lm->winfo;
        for(n = 0; n < r->result.sentnum; n++) { // �Z���e���X�̐��������[�v
            s = &(r->result.sent[n]);		     // �Z���e���X�̍\���̂̃A�h���X����
            seq = s->word;			     // ���[�h�i����i�����x���܂ŕ����������́j�̏W�܂��ID���擾
            seqnum = s->word_num;		     // ���[�h�̐�

            printf("����:", n+1);
            for(i=0;i<seqnum;i++) {	// ���[�h�̐�������
                //cc->convert(winfo->woutput[seq[i]], str_out, (size_t)STR_OUT); // ���[�h��utf8�ɕϊ�����str_out�ɓ����
                printf("%s", winfo->woutput[seq[i]]);	// �o��
            }
            printf("\n");
        }
    }
    //delete cc;
    fflush(stdout);
}


int main(int argc, char *argv[])
{
    Jconf *jconf;			// �ݒ�p�����[�^�̊i�[�ϐ�
    Recog *recog;			// �F���R�A�̃C���X�^���X�ϐ�
    int ret;

    jlog_set_output(NULL);	// ���O�@�\��OFF�ɂ���

    if (argc == 1) {		// -C�I�v�V�����Ŏw�肷��.jconf�t�@�C���͕K�v�Ȃ̂Ŋm�F����
        fprintf(stderr, "Try `-help' for more information.\n");
        return -1;
    }

    // �w�肵��.jconf�t�@�C������ݒ��ǂݍ���
    jconf = j_config_load_args_new(argc, argv);
    if (jconf == NULL) {
        return -1;
    }

    // �ǂݍ���jconf����F������쐬����
    recog = j_create_instance_from_jconf(jconf);
    if (recog == NULL) {
        fprintf(stderr, "Error in startup\n");
        return -1;
    }

    // �e��R�[���o�b�N�̍쐬
    // http://www.sp.nitech.ac.jp/~ri/julius-dev/doxygen/julius/4.1.2/ja/callback_8h.html
    callback_add(recog, CALLBACK_EVENT_SPEECH_READY, status_recready, NULL); // �����悤�ɂȂ�����status_recready���Ăяo��
    callback_add(recog, CALLBACK_RESULT, output_result, NULL); // �F������

    // �}�C�N�Ȃǂ̃I�[�f�B�I����̃C�j�V�����C�Y
    if (j_adin_init(recog) == FALSE) {    /* error */
        return -1;
    }

    // ���̓f�o�C�X�������ƊJ���Ă邩�m�F
    switch(j_open_stream(recog, NULL)) {
    case 0:			/* succeeded */
        break;
    case -1:      		/* error */
        fprintf(stderr, "error in input stream\n");
        return 0;
    case -2:			/* end of recognition process */
        fprintf(stderr, "failed to begin input stream\n");
        return 0;
    }

    // ���͂���F�����s�����[�v�ɓ���
    ret = j_recognize_stream(recog);
    if (ret == -1) return -1;	/* �����ŃG���[���N������ */
    // ret�̖߂�l��0�̏ꍇ�͐���I�������A���͂�S�ď������I���Ƃ������Ƃ̓}�C�N�ł͂Ȃ��Ǝv�����Ԃ�    

    // �I������
    j_close_stream(recog);	// �X�g���[�������
    j_recog_free(recog);		// �C���X�^���X���������

    return(0);
}
