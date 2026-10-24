#!/usr/bin/env python3
"""
PreToolUse hook for EUC-JP source file protection in Claude Code

このリポジトリのソースは EUC-JP で書かれている。標準の Read / Edit / Write は
UTF-8 前提のため、EUC-JP ファイルを読むと文字化けし、書き戻すと破損する。
そこで拡張子ではなく「中身」で EUC-JP を判定し、euc_* ツールへ誘導する。

Rules:
- Read : 非 ASCII を含む EUC-JP ファイルなら DENY
- Edit : 非 ASCII を含む EUC-JP ファイルなら DENY
         ソースファイルへ非 ASCII を持ち込む編集も DENY (UTF-8 が混入するため)
- Write: 既存の EUC-JP ファイルなら DENY
         ソースファイルを非 ASCII 入りで新規作成するのも DENY
- Bash/PowerShell: EUC-JP を化けさせるか改行コードを書き換えるコマンドを DENY

純 ASCII (0x00-0x7F のみ) のファイルは文字コードの区別が存在しないため素通しする。
UTF-8 のファイル (README.md など) は対象外で、標準ツールをそのまま使える。
"""
import json
import re
import sys
from pathlib import Path

ENCODING = 'euc_jis_2004'

# 非 ASCII の新規持ち込みを禁止するソースファイルの拡張子
SOURCE_EXTENSIONS = {'.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx'}

# シェル経由だと EUC-JP が化けるか改行が書き換わるコマンド群。
# (正規表現, 代替ツール名, 理由) の順。最小限に絞る（誤検知でユーザ操作を阻害しないため）。
SHELL_BLOCKED = [
    (re.compile(r'(?<![\w./-])sed(?![\w-])'), 'euc_sed',
     'sed は EUC-JP のロケール次第でマルチバイト文字を壊し、改行コードも書き換えます'),
    (re.compile(r'-replace(?![\w-])', re.IGNORECASE), 'euc_sed',
     'PowerShell の -replace はパイプ経由で EUC-JP が化け、改行が正規化されます'),
    (re.compile(r'(?<![\w./-])(od|xxd|hexdump|Format-Hex)(?![\w-])', re.IGNORECASE), 'euc_hexdump',
     'ダンプ結果がシェルの出力エンコーディングを経由するため信用できません'),
    (re.compile(r'certutil\b[^\n|;]*-encodehex', re.IGNORECASE), 'euc_hexdump',
     'certutil -encodehex は中間ファイルを経由し、確認したいバイト列がそのまま見えません'),
    (re.compile(r'-(Encoding\s+Byte|AsByteStream)(?![\w-])', re.IGNORECASE), 'euc_hexdump',
     'Get-Content のバイト読みは出力整形で情報が落ちます'),
]


def get_file_path(tool_input):
    return tool_input.get("file_path") or tool_input.get("path") or tool_input.get("filePath", "")


def deny(reason):
    output = {
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": "deny",
            "permissionDecisionReason": reason
        }
    }
    print(json.dumps(output))
    sys.exit(0)


def is_ascii_bytes(content):
    return all(b < 0x80 for b in bytearray(content))


def is_likely_euc(file_path):
    """非 ASCII を含み、UTF-8 ではなく EUC-JP として読めるファイルなら True。

    読めない・存在しない場合は False を返す (素通し側に倒す)。
    """
    try:
        content = Path(file_path).read_bytes()
    except Exception:
        return False
    if is_ascii_bytes(content):
        return False
    try:
        content.decode('utf-8')
        return False
    except UnicodeDecodeError:
        pass
    try:
        content.decode(ENCODING)
        return True
    except UnicodeDecodeError:
        return False


def handle_read(file_path, path_obj):
    if not path_obj.exists():
        sys.exit(0)
    if is_likely_euc(file_path):
        deny(
            f"このファイルは EUC-JP エンコードです: {path_obj.name}\n"
            f"\n"
            f"Read の代わりに euc_read ツールを使用してください。\n"
            f"  例: euc_read(file_path=\"{file_path}\")"
        )
    sys.exit(0)


def handle_edit(file_path, path_obj, tool_input):
    if is_likely_euc(file_path):
        deny(
            f"Edit は EUC-JP ファイルへの使用が禁止されています（エンコード破損防止）: {path_obj.name}\n"
            f"\n"
            f"Edit の代わりに euc_edit ツールを使用してください。\n"
            f"  例: euc_edit(file_path=\"{file_path}\", old_string=\"...\", new_string=\"...\")"
        )
    if path_obj.suffix.lower() in SOURCE_EXTENSIONS and path_obj.exists():
        new_string = tool_input.get("new_string", "")
        if isinstance(new_string, str) and not new_string.isascii():
            deny(
                f"{path_obj.name} は純 ASCII のため、標準の Edit で日本語を書き込むと UTF-8 になり、"
                f"このリポジトリの EUC-JP 規約を破ります。\n"
                f"\n"
                f"日本語を含める場合は euc_edit ツールを使用してください。"
            )
    sys.exit(0)


def handle_write(file_path, path_obj, tool_input):
    if path_obj.exists() and is_likely_euc(file_path):
        deny(
            f"既存の EUC-JP ファイルへの Write は禁止されています（エンコード破損防止）: {path_obj.name}\n"
            f"\n"
            f"Write の代わりに euc_write ツールを使用してください。\n"
            f"  例: euc_write(file_path=\"{file_path}\", content=\"...\")"
        )
    if path_obj.suffix.lower() in SOURCE_EXTENSIONS:
        content = tool_input.get("content", "")
        if isinstance(content, str) and not content.isascii():
            deny(
                f"{path_obj.name} に日本語を含めて標準の Write で書くと UTF-8 になり、"
                f"このリポジトリの EUC-JP 規約を破ります。\n"
                f"\n"
                f"euc_write ツールを使用してください。\n"
                f"  例: euc_write(file_path=\"{file_path}\", content=\"...\")"
            )
    sys.exit(0)


def handle_shell(command):
    # .claude 配下の自作ツール自体をメンテする場合は素通しする
    if '.claude' in command:
        sys.exit(0)
    for rx, alt, why in SHELL_BLOCKED:
        m = rx.search(command)
        if m:
            deny(
                f"シェルコマンド '{m.group(0)}' は使用禁止です（文字コード・改行コード破損防止）。\n"
                f"\n"
                f"理由: {why}\n"
                f"代わりに {alt} ツールを使用してください。"
            )
    sys.exit(0)


try:
    input_data = json.load(sys.stdin)
    tool_name = input_data.get("tool_name", "").lower()
    tool_input = input_data.get("tool_input", {})

    if tool_name in ("bash", "powershell"):
        command = tool_input.get("command", "")
        if command:
            handle_shell(command)
        sys.exit(0)

    file_path = get_file_path(tool_input)

    if not file_path:
        sys.exit(0)

    path_obj = Path(file_path)

    try:
        parts = path_obj.resolve().parts
    except Exception:
        parts = path_obj.parts
    if '.claude' in parts:
        sys.exit(0)

    if "read" in tool_name:
        handle_read(file_path, path_obj)
    elif "edit" in tool_name:
        handle_edit(file_path, path_obj, tool_input)
    elif "write" in tool_name:
        handle_write(file_path, path_obj, tool_input)

    sys.exit(0)

except Exception:
    sys.exit(0)
