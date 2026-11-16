//---------------------------------------------------------------------------
//
//  Kawari - KOSUI Adapter
//
//      Programed by Suikyo
//
//  2002.04.12  Phase 8.0.0   インターフェース切り直し
//  2026.10.18  Phase 9.0.0   wchar_t化
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#ifndef KAWARI_KOSUI_H
#define KAWARI_KOSUI_H
//---------------------------------------------------------------------------
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;
//---------------------------------------------------------------------------
#include "tool/kosui_base.h"
#include "libkawari/kawari_engine.h"
#include "libkawari/kawari_log.h"
#include "libkawari/kawari_version.h"
#include "misc/misc.h"
//---------------------------------------------------------------------------
// 華和梨＝幸水アダプタ
class TKawariKosuiAdapter: public TKawariInterface_base {
private:

	// 華和梨エンジン
	TKawariEngine engine;

public:

	TKawariKosuiAdapter(const wstring& datapath,const wstring& inifile)
	{
		// 乱数初期化
		engine.SRandom((unsigned int)time(NULL));

		engine.CreateEntry(L"System.DataPath").Push(engine.CreateStrWord(datapath));
		engine.WriteProtect(L"System.DataPath");

		// セットアップ
		engine.GetLogger().SetStreamStdOut();
		engine.SetDataPath(datapath);

		if (inifile.size()) {
			wstring legacyini;
			if (TKawariEngine::DetectLegacy7(datapath, inifile, legacyini)) {
				// 華和梨7の kawari.ini
				engine.SetLegacy7(true);
				engine.LoadKawari7Ini(legacyini);
			} else {
				engine.LoadKawariDict(inifile);
			}
		}
	}

	// 検査用 (kosui --check)
	// 安全モードにしてから辞書を読み込み、出た診断を collector に集計する。
	//  diagsink : 診断の出力先 (NULL なら出力しない)
	//  level    : 診断として出すレベル (loglevel 文では変えられない)
	//  loaded   : 起動する辞書を読めたか
	TKawariKosuiAdapter(const wstring& datapath, const wstring& inifile,
						wostream *diagsink, TDiagCollector *collector,
						unsigned int level, bool useseed, unsigned int seed, bool &loaded)
	{
		engine.SRandom(useseed ? seed : (unsigned int)time(NULL));

		// 辞書を読む前に、外へ影響する経路を全て止める
		engine.SetSandbox(true);
		engine.SetKeepZones(true);
		TKawariLogger &logger=engine.GetLogger();
		logger.SetStream(NULL);
		logger.SetDiagSink(diagsink);
		logger.SetDiagCollector(collector);
		logger.SetErrLevel(level);
		logger.LockLevel(true);

		engine.CreateEntry(L"System.DataPath").Push(engine.CreateStrWord(datapath));
		engine.WriteProtect(L"System.DataPath");
		engine.SetDataPath(datapath);

		loaded=true;
		if (inifile.size()) {
			wstring legacyini;
			if (TKawariEngine::DetectLegacy7(datapath, inifile, legacyini)) {
				engine.SetLegacy7(true);
				loaded=engine.LoadKawari7Ini(legacyini);
			} else {
				loaded=engine.LoadKawariDict(inifile);
			}
		}
	}

	~TKawariKosuiAdapter()
	{
		if (engine.IsLegacy7()) {
			// 華和梨7 : 切り離しイベント
			engine.RandomSelect(L"system.OnUnload");
			return;
		}
		TEntry entry=engine.GetEntry(L"System.Callback.OnUnload");
		unsigned int size=entry.Size();
		for(unsigned int i=0;i<size;i++) engine.Parse(entry.Index(i));
	}

	// 情報を得る (バージョン情報はASCIIのみ)
	virtual wstring GetInformation(void)
	{
		return AtoW(string(
			"Version : " KAWARI_NAME "/" KAWARI_MAJOR "." KAWARI_MINOR "." KAWARI_SUBMINOR "\n"
			"Kernel Version : " KAWARI_CORE_VERSION "  / " KAWARI_CORE_CODENAME "\n"
			KAWARI_CORE_CREDITS));
	}

	// 読み込んだ辞書を静的に検査する (検査用のコンストラクタで作ったときだけ意味がある)
	unsigned int Lint(void)
	{
		return engine.Lint();
	}

	// 与えられたスクリプトを解釈・実行する
	virtual wstring Parse(const wstring& script)
	{
		return(engine.Parse(script));
	}
};
//---------------------------------------------------------------------------
#endif // KAWARI_KOSUI_H
