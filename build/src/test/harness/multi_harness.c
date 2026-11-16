/*
 * multi_harness - tests the multi instance exports (multi_load / multi_loadu / multi_unload /
 * multi_request) together with the legacy load / request / unload of a SHIORI DLL.
 *
 * Usage:
 *   multi_harness <dll> <dir L> <dir A> <dir B>
 *
 * The three folders are copies of test/multi (the ghost). <dir L> is loaded with the legacy
 * load(), <dir A> and <dir B> with multi_load() / multi_loadu(). The program prints one line per
 * check, "ok <name>" or "NG <name> | <detail>", and the exit code is the number of failed checks
 * (0 = all passed). Exit codes 100 and over: 100 = bad usage, 101 = LoadLibrary failed,
 * 102 = export not found.
 *
 * Build with the same bitness as the DLL (the DLL is 32bit), for example:
 *   gcc -O1 -Wall -o multi_harness.exe multi_harness.c
 *
 * This file is ASCII only.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef BOOL (__cdecl *LOADF)(HGLOBAL, long);
typedef HGLOBAL (__cdecl *REQF)(HGLOBAL, long *);
typedef BOOL (__cdecl *UNLOADF)(void);
typedef long (__cdecl *MLOADF)(HGLOBAL, long);
typedef BOOL (__cdecl *MUNLOADF)(long);
typedef HGLOBAL (__cdecl *MREQF)(long, HGLOBAL, long *);

static LOADF p_load;
static REQF p_request;
static UNLOADF p_unload;
static MLOADF p_multi_load;
static MLOADF p_multi_loadu;
static MUNLOADF p_multi_unload;
static MREQF p_multi_request;

static int g_failed;
static const char *g_dir[3];	/* L, A, B */

#define LEGACY (-1000)	/* "id" meaning the legacy request() in ask() */
#define NSEQ 20

static void report(int ok, const char *name, const char *detail)
{
	if (ok) {
		printf("ok %s\n", name);
	} else {
		printf("NG %s | %s\n", name, detail ? detail : "");
		g_failed++;
	}
}

static HGLOBAL mkmem(const char *s, long *len)
{
	HGLOBAL h;
	*len = (long)strlen(s);
	h = GlobalAlloc(GMEM_FIXED, *len ? *len : 1);
	if (h) memcpy((void *)h, s, *len);
	return h;
}

static void fullpath(const char *dir, char *out, size_t outsz)
{
	size_t n;
	strncpy(out, dir, outsz - 2);
	out[outsz - 2] = 0;
	n = strlen(out);
	if (n && out[n - 1] != '\\' && out[n - 1] != '/') {
		out[n] = '\\';
		out[n + 1] = 0;
	}
}

/* legacy load() */
static int legacy_load(const char *dir)
{
	char path[1024];
	long len;
	HGLOBAL h;
	fullpath(dir, path, sizeof(path));
	h = mkmem(path, &len);
	return p_load(h, len) ? 1 : 0;
}

/* multi_load() (utf8=0) or multi_loadu() (utf8=1). returns the id, or -1 */
static long multi_load_dir(const char *dir, int utf8)
{
	char path[1024];
	long len;
	HGLOBAL h;
	fullpath(dir, path, sizeof(path));
	h = mkmem(path, &len);
	return utf8 ? p_multi_loadu(h, len) : p_multi_load(h, len);
}

/*
 * Sends "GET <id>" (with Reference0 if ref is not NULL) to the instance.
 * id == LEGACY : request(). otherwise : multi_request(id, ...).
 * Returns 1 when a response came back (its Value header goes to value), 0 when it was NULL.
 */
static int ask(long id, const char *evid, const char *ref, char *value, size_t vsz)
{
	char req[512];
	long len;
	HGLOBAL h, res;
	char *text;
	char *p;
	size_t n = 0;

	value[0] = 0;
	sprintf(req, "GET SHIORI/3.0\r\nCharset: UTF-8\r\nSender: SSP\r\nSecurityLevel: local\r\nID: %s\r\n", evid);
	if (ref) sprintf(req + strlen(req), "Reference0: %s\r\n", ref);
	strcat(req, "\r\n");
	h = mkmem(req, &len);
	if (id == LEGACY) res = p_request(h, &len);
	else res = p_multi_request(id, h, &len);
	if (!res) return 0;

	text = (char *)malloc((size_t)len + 1);
	memcpy(text, (void *)res, (size_t)len);
	text[len] = 0;
	GlobalFree(res);
	p = strstr(text, "\r\nValue: ");
	if (p) {
		p += 9;
		while (*p && *p != '\r' && n + 1 < vsz) value[n++] = *p++;
		value[n] = 0;
	}
	free(text);
	return 1;
}

static void seed(long id, int s)
{
	char v[64], r[16];
	sprintf(r, "%d", s);
	ask(id, "OnSeed", r, v, sizeof(v));
}

static void draw(long id, char *out, size_t sz)
{
	ask(id, "OnRand", NULL, out, sz);
}

static int answers(long id, const char *expect)
{
	char v[1024];
	if (!ask(id, "OnWho", NULL, v, sizeof(v))) return 0;
	return strstr(v, expect) != NULL;
}

static void marker_path(const char *dir, char *out, size_t sz)
{
	fullpath(dir, out, sz);
	strncat(out, "unloaded.txt", sz - strlen(out) - 1);
}

static void marker_clear(void)
{
	char p[1024];
	int i;
	for (i = 0; i < 3; i++) {
		marker_path(g_dir[i], p, sizeof(p));
		DeleteFileA(p);
	}
}

static int marker_exists(const char *dir)
{
	char p[1024];
	marker_path(dir, p, sizeof(p));
	return GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES;
}

static void *must_get(HMODULE dll, const char *name)
{
	void *p = (void *)GetProcAddress(dll, name);
	if (!p) {
		fprintf(stderr, "export not found: %s\n", name);
		exit(102);
	}
	return p;
}

int main(int argc, char **argv)
{
	HMODULE dll;
	static char base[NSEQ][32];
	static char sq[3][NSEQ][32];
	char v[256];
	long a, b, c;
	int i, k, same;

	if (argc != 5) {
		fprintf(stderr, "usage: multi_harness <dll> <dir L> <dir A> <dir B>\n");
		return 100;
	}
	g_dir[0] = argv[2];
	g_dir[1] = argv[3];
	g_dir[2] = argv[4];
	dll = LoadLibraryA(argv[1]);
	if (!dll) return 101;
	p_load = (LOADF)must_get(dll, "load");
	p_request = (REQF)must_get(dll, "request");
	p_unload = (UNLOADF)must_get(dll, "unload");
	p_multi_load = (MLOADF)must_get(dll, "multi_load");
	p_multi_loadu = (MLOADF)must_get(dll, "multi_loadu");
	p_multi_unload = (MUNLOADF)must_get(dll, "multi_unload");
	p_multi_request = (MREQF)must_get(dll, "multi_request");

	/* ---- baseline: one legacy instance alone, seeded with 42 ---- */
	report(legacy_load(g_dir[0]), "baseline: load", "load() failed");
	seed(LEGACY, 42);
	for (i = 0; i < NSEQ; i++) draw(LEGACY, base[i], sizeof(base[i]));
	report(base[0][0] != 0, "baseline: rand answers", "empty value");
	report(p_unload() != 0, "baseline: unload", "unload() returned FALSE");

	/* ---- three instances (legacy + 2 multi), loaded after the others were seeded ---- */
	report(legacy_load(g_dir[0]), "rng: load legacy", "load() failed");
	seed(LEGACY, 42);
	a = multi_load_dir(g_dir[1], 0);
	report(a >= 1, "rng: multi_load A returns id >= 1", "id");
	seed(a, 42);
	b = multi_load_dir(g_dir[2], 1);
	report(b >= 1 && b != a, "rng: multi_loadu B returns another id >= 1", "id");
	seed(b, 42);
	for (i = 0; i < NSEQ; i++) {
		draw(LEGACY, sq[0][i], sizeof(sq[0][i]));
		draw(a, sq[1][i], sizeof(sq[1][i]));
		draw(b, sq[2][i], sizeof(sq[2][i]));
	}
	for (k = 0; k < 3; k++) {
		same = 1;
		for (i = 0; i < NSEQ; i++) if (strcmp(sq[k][i], base[i]) != 0) same = 0;
		report(same, k == 0 ? "rng: legacy sequence equals the single run" :
			k == 1 ? "rng: A sequence equals the single run" : "rng: B sequence equals the single run",
			"the sequence differs from the one of a lone instance");
	}
	/* different seeds do not disturb each other */
	seed(a, 42);
	seed(b, 7);
	seed(LEGACY, 7);
	same = 1;
	for (i = 0; i < NSEQ; i++) {
		draw(b, v, sizeof(v));
		draw(LEGACY, v, sizeof(v));
		draw(a, v, sizeof(v));
		if (strcmp(v, base[i]) != 0) same = 0;
	}
	report(same, "rng: A keeps its sequence while the others use another seed", "A was disturbed");

	/* ---- each instance answers for itself ---- */
	report(answers(LEGACY, "ghost_L"), "isolation: legacy answers with its own folder", "wrong folder");
	report(answers(a, "ghost_A"), "isolation: A answers with its own folder", "wrong folder");
	report(answers(b, "ghost_B"), "isolation: B answers with its own folder", "wrong folder");

	/* ---- invalid ids ---- */
	report(!ask(0, "OnWho", NULL, v, sizeof(v)), "invalid: multi_request(0) is NULL", "got a response");
	report(!ask(-1, "OnWho", NULL, v, sizeof(v)), "invalid: multi_request(-1) is NULL", "got a response");
	report(!ask(b + 100, "OnWho", NULL, v, sizeof(v)), "invalid: multi_request(unknown id) is NULL", "got a response");
	report(p_multi_unload(0) == 0, "invalid: multi_unload(0) is FALSE", "TRUE");
	report(p_multi_unload(-5) == 0, "invalid: multi_unload(-5) is FALSE", "TRUE");
	report(p_multi_unload(b + 100) == 0, "invalid: multi_unload(unknown id) is FALSE", "TRUE");
	report(answers(LEGACY, "ghost_L") && answers(a, "ghost_A") && answers(b, "ghost_B"),
		"invalid: the failed calls did not touch the instances", "an instance stopped answering");

	/* ---- order 1: legacy first ---- */
	marker_clear();
	report(p_unload() != 0, "order1: unload() legacy", "FALSE");
	report(answers(a, "ghost_A") && answers(b, "ghost_B"), "order1: A and B still answer", "an instance is gone");
	report(!ask(LEGACY, "OnWho", NULL, v, sizeof(v)) || v[0] == 0, "order1: legacy request after unload gives no answer", "answered");
	report(p_multi_unload(a) != 0, "order1: multi_unload(A)", "FALSE");
	report(answers(b, "ghost_B"), "order1: B still answers", "B is gone");
	report(!ask(a, "OnWho", NULL, v, sizeof(v)), "order1: unloaded id A is NULL", "got a response");
	report(p_multi_unload(a) == 0, "order1: second multi_unload(A) is FALSE", "TRUE");
	report(p_multi_unload(b) != 0, "order1: multi_unload(B)", "FALSE");
	report(marker_exists(g_dir[0]) && marker_exists(g_dir[1]) && marker_exists(g_dir[2]),
		"order1: OnUnload ran for every instance", "a marker file is missing");

	/* ---- order 2: a multi instance first, legacy in the middle ---- */
	marker_clear();
	report(legacy_load(g_dir[0]), "order2: load legacy", "FALSE");
	a = multi_load_dir(g_dir[1], 0);
	b = multi_load_dir(g_dir[2], 0);
	c = multi_load_dir(g_dir[1], 0);	/* a second instance of the same folder */
	report(a >= 1 && b >= 1 && c >= 1 && a != b && a != c && b != c, "order2: three distinct ids", "ids");
	report(p_multi_unload(a) != 0, "order2: multi_unload(A)", "FALSE");
	report(answers(LEGACY, "ghost_L") && answers(b, "ghost_B") && answers(c, "ghost_A"),
		"order2: legacy, B and the second A still answer", "an instance is gone");
	/* the freed id may be reused */
	a = multi_load_dir(g_dir[2], 0);
	report(a >= 1 && answers(a, "ghost_B"), "order2: a freed id can be reused by a new instance", "failed");
	report(p_unload() != 0, "order2: unload() legacy", "FALSE");
	report(answers(a, "ghost_B") && answers(b, "ghost_B") && answers(c, "ghost_A"),
		"order2: the multi instances survive unload()", "an instance is gone");
	report(p_multi_unload(c) != 0 && p_multi_unload(b) != 0 && p_multi_unload(a) != 0,
		"order2: multi_unload of the rest", "FALSE");

	/* ---- only a legacy instance: no multi id reaches it ---- */
	report(legacy_load(g_dir[0]), "legacy only: load", "FALSE");
	report(!ask(1, "OnWho", NULL, v, sizeof(v)), "legacy only: multi_request(1) does not reach the legacy instance", "got a response");
	report(!ask(2, "OnWho", NULL, v, sizeof(v)), "legacy only: multi_request(2) is NULL", "got a response");
	report(p_multi_unload(1) == 0, "legacy only: multi_unload(1) does not unload the legacy instance", "TRUE");
	report(answers(LEGACY, "ghost_L"), "legacy only: the legacy instance still answers", "gone");
	report(p_unload() != 0, "legacy only: unload", "FALSE");

	/* ---- everything unloaded: loading again works ---- */
	a = multi_load_dir(g_dir[1], 1);
	report(a >= 1 && answers(a, "ghost_A"), "reload: multi_loadu after everything was unloaded", "failed");
	report(p_multi_unload(a) != 0, "reload: multi_unload", "FALSE");

	FreeLibrary(dll);
	printf("multi: %d failed\n", g_failed);
	return g_failed > 99 ? 99 : g_failed;
}
