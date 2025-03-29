#include <string.h>

const char *lang_code;

const char *translation_gettext(const char *msg)
{
    if (strcmp(msg, "Linguine CLI Version 0.0.2\n") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "Linguine コマンドライン バージョン 0.0.2\n";
        return "Linguine CLI Version 0.0.2\n";
    }
    if (strcmp(msg, "Usage:\n  Run program:\n    linguine <source files and/or bytecode files>\n  Run program (safe mode):\n    linguine --safe-mode <source files and/or bytecode files>\n  Compile to a bytecode file:\n    linguine --bytecode <source files>\n  Compile to an application C source:\n    linguine --app <source files>\n  Compile to a DLL C source:\n    linguine --dll <source files>\n  Show this help:\n    linguine --help\n  Show version:\n    linguine --version\n") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "使い方:\n  スクリプトを実行するm:\n    linguine <ソースファイル / バイトコードファイル>\n  スクリプトを実行する（セーフモード）:\n    linguine --safe-mode <ソースファイル / バイトコードファイル>\n  バイトコードファイルに変換する:\n    linguine --bytecode <ソースファイル>\n  アプリのCソースに変換する:\n    linguine --app <ソースファイル>\n  DLLのCソースに変換する:\n    linguine --dll <ソースファイル>\n  このヘルプを表示する:\n    linguine --help\n  バージョンを表示する:\n    linguine --version\n";
        return "Usage:\n  Run program:\n    linguine <source files and/or bytecode files>\n  Run program (safe mode):\n    linguine --safe-mode <source files and/or bytecode files>\n  Compile to a bytecode file:\n    linguine --bytecode <source files>\n  Compile to an application C source:\n    linguine --app <source files>\n  Compile to a DLL C source:\n    linguine --dll <source files>\n  Show this help:\n    linguine --help\n  Show version:\n    linguine --version\n";
    }
    if (strcmp(msg, "syntax error") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "文法エラーです。";
        return "syntax error";
    }
    if (strcmp(msg, "%s: Out of memory while parsing.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "%s: メモリが足りません。";
        return "%s: Out of memory while parsing.";
    }
    if (strcmp(msg, "Too many functions.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "関数が多すぎます。";
        return "Too many functions.";
    }
    if (strcmp(msg, "continue appeared outside loop.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "continue文がループの外で使用されました。";
        return "continue appeared outside loop.";
    }
    if (strcmp(msg, "LHS is not a term or an array element.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "左辺が項か配列要素ではありません。";
        return "LHS is not a term or an array element.";
    }
    if (strcmp(msg, "else-if block appeared without if block.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "else if文がifブロックの後ろ以外で使用されました。";
        return "else-if block appeared without if block.";
    }
    if (strcmp(msg, "else-if appeared after else.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "else if文がelse文の後ろで使用されました。";
        return "else-if appeared after else.";
    }
    if (strcmp(msg, "else appeared after else.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "else文がelse文の後ろで使用されました。";
        return "else appeared after else.";
    }
    if (strcmp(msg, "Exceeded the maximum argument count.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "引数が多すぎます。";
        return "Exceeded the maximum argument count.";
    }
    if (strcmp(msg, "Too many anonymous functions.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "無名関数が多すぎます。";
        return "Too many anonymous functions.";
    }
    if (strcmp(msg, "LHS is not a symbol or an array element.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "左辺がシンボルか配列要素ではありません。";
        return "LHS is not a symbol or an array element.";
    }
    if (strcmp(msg, "Too much local variables.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "ローカル変数が多すぎます。";
        return "Too much local variables.";
    }
    if (strcmp(msg, "Too many jumps.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "ジャンプが多すぎます。";
        return "Too many jumps.";
    }
    if (strcmp(msg, "LIR: Out of memory error.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "LIR: メモリが足りません。";
        return "LIR: Out of memory error.";
    }
    if (strcmp(msg, "Memory mapping failed.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "メモリマップに失敗しました。";
        return "Memory mapping failed.";
    }
    if (strcmp(msg, "Code too big.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "コードが大きすぎます。";
        return "Code too big.";
    }
    if (strcmp(msg, "Branch target not found.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "分岐先がみつかりません。";
        return "Branch target not found.";
    }
    if (strcmp(msg, "Failed to load bytecode.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "バイトコードの読み込みに失敗しました。";
        return "Failed to load bytecode.";
    }
    if (strcmp(msg, "Cannot find function.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "関数がみつかりません。";
        return "Cannot find function.";
    }
    if (strcmp(msg, "Not an array.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "配列ではありません。";
        return "Not an array.";
    }
    if (strcmp(msg, "Array index %d is out-of-range.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "配列の添字 %d は範囲外です。";
        return "Array index %d is out-of-range.";
    }
    if (strcmp(msg, "Dictionary key \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "辞書のキー \"%s\" がみつかりません。";
        return "Dictionary key \"%s\" not found.";
    }
    if (strcmp(msg, "Local variable \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "ローカル変数 \"%s\" がみつかりません。";
        return "Local variable \"%s\" not found.";
    }
    if (strcmp(msg, "Global variable \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "グローバル変数 \"%s\" がみつかりません。";
        return "Global variable \"%s\" not found.";
    }
    if (strcmp(msg, "Value is not a number.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "値が数ではありません。";
        return "Value is not a number.";
    }
    if (strcmp(msg, "Value is not an integer.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "値が整数ではありません。";
        return "Value is not an integer.";
    }
    if (strcmp(msg, "Value is not a string.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "値が文字列ではありません。";
        return "Value is not a string.";
    }
    if (strcmp(msg, "Value is not a number or a string.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "値が数か文字列ではありません。";
        return "Value is not a number or a string.";
    }
    if (strcmp(msg, "Division by zero.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "ゼロによる除算です。";
        return "Division by zero.";
    }
    if (strcmp(msg, "Subscript not an integer.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "添字が整数ではありません。";
        return "Subscript not an integer.";
    }
    if (strcmp(msg, "Not an array or a dictionary.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "配列か辞書ではありません。";
        return "Not an array or a dictionary.";
    }
    if (strcmp(msg, "Subscript not a string.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "添字が文字列ではありません。";
        return "Subscript not a string.";
    }
    if (strcmp(msg, "Value is not a string, an array, or a dictionary.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "値が文字列、配列、辞書ではありません。";
        return "Value is not a string, an array, or a dictionary.";
    }
    if (strcmp(msg, "Not a dictionary.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "辞書ではありません。";
        return "Not a dictionary.";
    }
    if (strcmp(msg, "Dictionary index out-of-range.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "辞書のインデックスが範囲外です。";
        return "Dictionary index out-of-range.";
    }
    if (strcmp(msg, "Symbol \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "シンボル \"%s\" がみつかりません。";
        return "Symbol \"%s\" not found.";
    }
    if (strcmp(msg, "Not a function.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "関数ではありません。";
        return "Not a function.";
    }
    if (strcmp(msg, "Out of memory.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "メモリが足りません。";
        return "Out of memory.";
    }
    if (strcmp(msg, "Error: %s: %d: %s\n") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "エラー: %s: %d: %s\n";
        return "Error: %s: %d: %s\n";
    }
    if (strcmp(msg, "Cannot open file \"%s\".\n") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "ファイル \"%s\" を開けません。\n";
        return "Cannot open file \"%s\".\n";
    }
    if (strcmp(msg, "Cannot read file \"%s\".\n") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "ファイル \"%s\" を読み込めません。\n";
        return "Cannot read file \"%s\".\n";
    }
    if (strcmp(msg, "%s:%d: error: %s\n") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "%s:%d: エラー: %s\n";
        return "%s:%d: error: %s\n";
    }
    return msg;
}
