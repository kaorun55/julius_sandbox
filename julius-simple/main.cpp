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

// 喋れるようになったら呼び出されるコールバック
static void status_recready(Recog *recog, void *dummy)
{
    if (recog->jconf->input.speech_input == SP_MIC || recog->jconf->input.speech_input == SP_NETAUDIO) {
        fprintf(stderr, "<<< please speak >>>\n");
    }
}


// 最終的な認識結果をアウトプットするコールバック
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

        // 処理が生きてないときは飛ばす（どんな時かは知らないけど）
        if (! r->live) continue;

        // 処理結果が得られない時も飛ばす（どんな時だそれ）
        if (r->result.status < 0) continue;

        // 全てのセンテンスをアウトプットする
        winfo = r->lm->winfo;
        for(n = 0; n < r->result.sentnum; n++) { // センテンスの数だけループ
            s = &(r->result.sent[n]);		     // センテンスの構造体のアドレス入手
            seq = s->word;			     // ワード（文を品詞レベルまで分解したもの）の集まりのIDを取得
            seqnum = s->word_num;		     // ワードの数

            printf("結果:", n+1);
            for(i=0;i<seqnum;i++) {	// ワードの数だけ回す
                //cc->convert(winfo->woutput[seq[i]], str_out, (size_t)STR_OUT); // ワードをutf8に変換してstr_outに入れる
                printf("%s", winfo->woutput[seq[i]]);	// 出力
            }
            printf("\n");
        }
    }
    //delete cc;
    fflush(stdout);
}


int main(int argc, char *argv[])
{
    Jconf *jconf;			// 設定パラメータの格納変数
    Recog *recog;			// 認識コアのインスタンス変数
    int ret;

    jlog_set_output(NULL);	// ログ機能をOFFにする

    if (argc == 1) {		// -Cオプションで指定する.jconfファイルは必要なので確認する
        fprintf(stderr, "Try `-help' for more information.\n");
        return -1;
    }

    // 指定した.jconfファイルから設定を読み込む
    jconf = j_config_load_args_new(argc, argv);
    if (jconf == NULL) {
        return -1;
    }

    // 読み込んだjconfから認識器を作成する
    recog = j_create_instance_from_jconf(jconf);
    if (recog == NULL) {
        fprintf(stderr, "Error in startup\n");
        return -1;
    }

    // 各種コールバックの作成
    // http://www.sp.nitech.ac.jp/~ri/julius-dev/doxygen/julius/4.1.2/ja/callback_8h.html
    callback_add(recog, CALLBACK_EVENT_SPEECH_READY, status_recready, NULL); // 喋れるようになったらstatus_recreadyを呼び出す
    callback_add(recog, CALLBACK_RESULT, output_result, NULL); // 認識結果

    // マイクなどのオーディオ周りのイニシャライズ
    if (j_adin_init(recog) == FALSE) {    /* error */
        return -1;
    }

    // 入力デバイスがちゃんと開けてるか確認
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

    // 入力から認識を行うループに入る
    ret = j_recognize_stream(recog);
    if (ret == -1) return -1;	/* 内部でエラーが起こった */
    // retの戻り値が0の場合は正常終了だが、入力を全て処理し終わるということはマイクではないと思うたぶん    

    // 終了処理
    j_close_stream(recog);	// ストリームを閉じる
    j_recog_free(recog);		// インスタンスを解放する

    return(0);
}
