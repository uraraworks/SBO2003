#pragma once
#include <string>
#include <string.h>
/* 番外編の文言。置換値に含まれる印は再展開しない。 */
struct DungeonTextDefault { const char *key; const char *text; };
static const DungeonTextDefault g_aDungeonTextDefault[] = {
    {"reception", "経験値・お金は入りません。拾った物は中で使えますが、外へは持ち帰れません。姿を選んでお入りください。"},
    {"select_title", "どの姿で入る？"},
    {"member_guide", "経験値・お金は入りません。拾った物は中で使えますが、外へは持ち帰れません。姿を選んでお入りください。"},
    {"not_ready", "このダンジョンは準備中です。しばらくしてからもう一度どうぞ。"},
    {"no_forms", "このダンジョンは準備中です。"},
    {"invalid_reception", "この受付では申し込めません。"},
    {"ready", "準備ができました。そこの入口に入ると開始です。いってらっしゃいませ。"},
    {"ready_timeout", "準備完了から2分が過ぎたので、組の受付を取り消しました。もう一度受付で申し込んでください。"},
    {"ready_cancelled", "仲間の離脱・切断・移動、または状態が変わったため、組の受付を取り消しました。もう一度受付で申し込んでください。"},
    {"party_ready", "この組は受付済みです。準備ができた仲間が入口に入ると開始です。"},
    {"ready_gather", "{name}さんが入口の近くにいません。全員そろってから入口に入ってください。"},
    {"entrance", "受付で申し込んでください。"},
    {"leader_only", "パーティのリーダーが受付の人に話しかけると、みんなで入れます。"},
    {"mercenary", "{name}さんの傭兵は一緒に入れません。別れてから来てください。"},
    {"mercenary_self", "傭兵は一緒に入れません。別れてから来てください。"},
    {"party_selecting", "今{name}さんの組が姿を選んでいます。"},
    {"party_running", "パーティの仲間がすでに挑戦中です。その組が終わってから、あらためて受付で申し込んでください。"},
    {"joined", "{name}さんの組に加わりました。残り{seconds}秒で姿を選んでください。"},
    {"member_joined", "{name}さんが組に加わりました。"},
    {"running", "今{count}組が挑戦中です。その組と同じ形のダンジョンに入ります。"},
    {"empty", "今は誰も入っていません。"},
    {"far_members", "近くにいない仲間：{names}（締め切りまでに受付の近くへ来れば一緒に入れます）"},
    {"far_name", "{name}さん"},
    {"name_separator", "、"},
    {"selection_timeout", "時間内に姿を選ばなかったので、置いていかれました。"},
    {"moved_away", "入口から離れたので、ダンジョンには入れませんでした。"},
    {"cancel", "ダンジョンに入るのをやめました。"},
    {"entered", "ダンジョンに入りました（{name}）。経験値・お金は入りません。拾った物は中で使えますが、外へは持ち帰れません。"},
    {"leave_clear", "クリアしました！ 元の姿に戻ります。"},
    {"leave_dead", "力尽きてしまった…。元の姿に戻りました。"},
    {"leave_party", "パーティを抜けたので、ダンジョンから出ました。元の姿に戻りました。"},
    {"leave_out", "ダンジョンの外へ出たので、元の姿に戻りました。"},
    {"leave_ask", "ダンジョンから抜けますか？"},
    {"leave_retire", "途中で帰りました（記録なし）。元の姿に戻りました。"},
    {"leave_build", "ダンジョンが作り直されたので外へ出ました。元の姿に戻りました。"},
    {"leave_end", "ダンジョンの挑戦が終わりました。元の姿に戻りました。"},
    {"all_dead", "力尽きてしまった…。元の姿に戻りました。"},
    {"remaining", "残り {count} 体"},
    {"killall_clear", "全部倒しました！ {result}。10秒後に全員戻ります。"},
    {"countdown", "{seconds}"},
    {"clear_time", "クリア時間 {time}"},
    {"unranked", "（今日の挑戦は2回目以降の人がいるので、順位には入りません）"},
    {"rank_best", "（今日の{rank}位）"},
    {"rank_unchanged", "（この顔ぶれの今日の最高は{rank}位。記録は更新されませんでした）"},
    {"rank_out", "（今日の記録の{limit}位には入れませんでした）"},
    {"daily_changed", "日付が変わったので、今の組が出たら新しいダンジョンに作り直します。"},
    {"daily_first", "今日1回目の挑戦です。クリアすると順位に入ります。"},
    {"daily_again", "今日はもう挑戦したので、今回はタイムだけです（順位には入りません）。"},
    {"records_empty", "今日の記録はまだありません。"},
    {"records_title", "今日の記録 "},
    {"record_entry", "{rank}位 {time}{names}"},
    {"record_names", "（{name}{others}）"},
    {"record_others", "ほか{count}人"},
    {"record_separator", " / "},
    {"rebuild_notice", "管理者がダンジョンを作り直します。外へ戻ります。"},
    {"blocked", "ダンジョンの中ではできません。"},
    {"def_base_name", "拠点"},
    {"def_spring_name", "回復の泉"},
    {"def_wave_coming", "ウェーブ {wave}/{total} が来ます（{seconds}秒後）"},
    {"def_wave_start", "ウェーブ {wave} 開始"},
    {"def_wave_clear", "ウェーブ {wave} をしのいだ！"},
    {"def_enemy_left", "残り敵 {count}"},
    {"def_base_attacked", "拠点が攻撃されています（HP {percent}%）"},
    {"def_base_fallen", "拠点が落ちた…。10秒後に全員戻ります。"},
    {"def_win", "全部のウェーブをしのいだ！ 10秒後に全員戻ります。"},
    {"def_down", "倒れた…。{seconds}秒後に復活します。"},
    {"def_revived", "復活しました。"},
};
inline std::string DungeonText(const SUBGAME_DUNGEONINFO *p, const char *key,
    const char *k1 = "", const std::string &v1 = "", const char *k2 = "", const std::string &v2 = "",
    const char *k3 = "", const std::string &v3 = "")
{
    std::string text;
    for (size_t i = 0; i < sizeof (g_aDungeonTextDefault) / sizeof (g_aDungeonTextDefault[0]); i ++)
        if (strcmp (key, g_aDungeonTextDefault[i].key) == 0) { text = g_aDungeonTextDefault[i].text; break; }
    if (p) {
        size_t len = 0;
        while (len < sizeof (p->szMessages) && p->szMessages[len]) len ++;
        std::string all (p->szMessages, len);
        for (size_t at = 0; at < all.size (); ) {
            size_t end = all.find ('\n', at);
            if (end == std::string::npos) end = all.size ();
            std::string line = all.substr (at, end - at);
            if (!line.empty () && line[line.size () - 1] == '\r') line.resize (line.size () - 1);
            size_t eq = line.find ('=');
            if (eq != std::string::npos && line.substr (0, eq) == key && eq + 1 < line.size ()) text = line.substr (eq + 1);
            at = end + 1;
        }
    }
    std::string out;
    for (size_t i = 0; i < text.size (); ) {
        if (text.compare (i, 2, "\\n") == 0) { out += '\n'; i += 2; continue; }
        const char *keys[] = {k1, k2, k3};
        const std::string *values[] = {&v1, &v2, &v3};
        bool found = false;
        for (int j = 0; j < 3; j ++) {
            if (!*keys[j]) continue;
            std::string mark = "{" + std::string (keys[j]) + "}";
            if (text.compare (i, mark.size (), mark) == 0) { out += *values[j]; i += mark.size (); found = true; break; }
        }
        if (!found) out += text[i ++];
    }
    /* 差し込み後も通信の上限内に収め、文字の途中では切らない。 */
    char bounded[MAX_TALKMSG];
    SboTextCopy (bounded, sizeof (bounded), out.c_str ());
    return bounded;
}
