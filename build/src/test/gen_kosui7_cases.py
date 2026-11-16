"""Generate build/src/test/kosui7/cases.txt: the expected outputs come from the REAL Kawari 7.5.0 kosui.

usage: python gen_cases7.py <kosui 7.5.0 exe> <fixture dir> <output cases.txt>
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

KOSUI7, FIXTURE, OUT = sys.argv[1], sys.argv[2], sys.argv[3]

# (section comment or None, verb, text). Texts are ASCII. The expected output is taken from kosui 7.5.0.
CASES = [
    ('entry reference', None, None),
    ('echo', '${greet}'), ('echo', '${nest}'), ('echo', 'plain text'), ('echo', '${nosuch}'),
    ('echo', '${ b }'), ('echo', '${one}${one}'),
    ('entry names are flat strings in 7: "dots" "dots." "dots..x" are three different entries', None, None),
    ('echo', '${dots}'), ('echo', '${dots.}'), ('echo', '${dots..x}'), ('echo', '${dots.x}'),
    ('@ is an ordinary character of an entry name in 7 (not a local entry as in 9)', None, None),
    ('cmd', 'set @loc 5 ; echo ${@loc}'), ('cmd', 'set @loc.a 1 ; echo ${@loc.a}/${@loc}'), ('echo', '${@none}'),
    ('lexical rules: separators are plain text in a sentence, a quoted text is not expanded', None, None),
    ('echo', 'a (b) c'), ('echo', 'a;b:c!d&e|f'), ('echo', 'a,b'), ('echo', '"he said \\"hi\\""'),
    ('echo', '$x $() $'), ('echo', 'a   b'), ('echo', '${quote}'), ('echo', '"${greet}"'),
    ('echo', '$(echo a'),
    ('history reference counts only the entry calls (${N} is not counted, $() is not counted)', None, None),
    ('echo', '${b}$(echo q)${0}/${1}'), ('echo', '${b}${n}${1}/${0}'), ('echo', '${b}${n}${0}${1}${2}'),
    ('echo', '$(echo ${b} ${n})${0}${1}'),
    ('if takes a condition, a word and an optional else word (no else keyword)', None, None),
    ('echo', '$(if 1 yes no)'), ('echo', '$(if "" yes no)'), ('echo', '$(if 0 yes)'), ('echo', '$(if 0 yes no)'),
    ('echo', '$(if false yes no)'), ('echo', '$(if 1 yes else no)'), ('echo', '$(if 1 $(echo x)$(echo y) z)'),
    ('cmd', 'if 1 a b'), ('cmd', 'if x a'),
    ('control commands', None, None),
    ('cmd', 'loop 3 x'), ('cmd', 'loop 0 x'), ('cmd', 'loop -1 x'), ('cmd', 'while "" x'), ('cmd', 'until 1 x'),
    ('cmd', '? a'), ('cmd', 'NULL'), ('cmd', 'echo 1 ; echo 2 ; silent ; echo 3'), ('cmd', 'foreach i one ${i}!'),
    ('the arguments of a statement', None, None),
    ('cmd', 'echo a  b   c'), ('cmd', 'echo "a  b"'), ('cmd', 'echo a;b'), ('cmd', 'echo (a)'),
    ('cmd', 'echo 1 ; echo 2'), ('cmd', 'echo'),
    ('expr (7.5.0 style: each token is an argument)', None, None),
    ('cmd', 'expr 1 + 2'), ('cmd', 'expr 5 / 2'), ('cmd', 'expr 5 % 3'), ('cmd', 'expr 5 / 0'), ('cmd', 'expr 7 - 10'),
    ('cmd', 'expr 3 * 4'), ('cmd', 'expr 8 / 4 / 2'), ('cmd', 'expr 1 + 2 * 3'), ('cmd', 'expr ( 1 + 2 ) * 3'),
    ('cmd', 'expr 2 * ( 1 + 2 ")" '), ('cmd', 'expr abc + 1'), ('cmd', 'expr 5x + 1'),
    ('cmd', 'expr 3 > 2'), ('cmd', 'expr 3 < 2'), ('cmd', 'expr 3 = 3'), ('cmd', 'expr 3 == 3'), ('cmd', 'expr 3 != 3'),
    ('cmd', 'expr 3 >= 3'), ('cmd', 'expr 3 <= 2'), ('cmd', 'expr abc = abc'), ('cmd', 'expr abc < abd'),
    ('cmd', 'expr 10 < 9'), ('cmd', 'expr 1 | 2'), ('cmd', 'expr "" | 2'), ('cmd', 'expr 1 & 0'), ('cmd', 'expr 1 & 5'),
    ('cmd', 'expr substr abcdef 2 3'), ('cmd', 'expr substr abcdef 0 3'), ('cmd', 'expr substr abcdef 7 3'),
    ('cmd', 'expr length abcd'), ('cmd', 'expr index abcdef xcd'), ('cmd', 'expr index abcdef zz'),
    ('cmd', 'expr find abcdef cd'), ('cmd', 'expr find abcdef zz'), ('cmd', 'expr findpos abcdef cd'),
    ('cmd', 'expr findpos abcdef zz'), ('cmd', 'expr quote find'), ('cmd', 'expr 1 +'), ('cmd', 'expr'),
    ('cmd', 'expr ${n} + ${n}'),
    ('test and [ ]', None, None),
    ('cmd', 'test 1'), ('cmd', 'test 0'), ('cmd', 'test ""'), ('cmd', 'test false'), ('cmd', 'test ! 0'),
    ('cmd', 'test ! x'), ('cmd', 'test a == a'), ('cmd', 'test a = b'), ('cmd', 'test a != b'), ('cmd', 'test 2 < 10'),
    ('cmd', 'test 2 -lt 10'), ('cmd', 'test 2 -gt 10'), ('cmd', 'test 10 -ge 10'), ('cmd', 'test 3 -eq 3'),
    ('cmd', 'test 3 -ne 3'), ('cmd', 'test 1 && 0'), ('cmd', 'test 1 && 1'), ('cmd', 'test 0 || x'), ('cmd', 'test a b'),
    ('cmd', '[ 1 -eq 1 ]'), ('cmd', '[ ${n} -eq 3 ]'), ('cmd', '[ a == b ]'), ('echo', '$([ ${n} -lt ${ten} ])'),
    ('dictionary commands (7.5.0: get shows the source, array runs it, enumerate lists the source)', None, None),
    ('cmd', 'get lst 0'), ('cmd', 'get lst 2'), ('cmd', 'get lst 9'), ('cmd', 'get lst'), ('cmd', 'get nest 0'),
    ('cmd', 'array lst 1'), ('cmd', 'array nest 0'), ('cmd', 'array lst 9'), ('cmd', 'enumerate lst'),
    ('cmd', 'enumerate nest'), ('cmd', 'enumerate quote'), ('cmd', 'enumerate nosuch'),
    ('cmd', 'size lst'), ('cmd', 'size nosuch'), ('cmd', 'size dots.'),
    ('cmd', 'set x1 abc ; echo ${x1}'), ('cmd', 'set x2 "${greet}" ; echo ${x2}'), ('cmd', 'set x3 "" ; size x3'),
    ('cmd', 'adddict x4 one ; adddict x4 two ; size x4'), ('cmd', 'set x5 a ; clear x5 ; size x5'),
    ('cmd', 'set x6 "a b" ; array x6 0'), ('cmd', 'set x7 "(q)" ; echo ${x7}'),
    ('cmd', 'set x8 "a ; b" ; get x8 0'),
    ('cmd', 'entry one'), ('cmd', 'entry nosuch dflt'), ('cmd', 'entry nosuch'), ('cmd', 'eval "${greet}"'),
    ('cmd', 'eval "$(echo e)"'), ('cmd', 'eval a b'),
    ('counters and strings', None, None),
    ('cmd', 'set c 5 ; dec c ; echo ${c}'), ('cmd', 'set c 5 ; inc c 3 ; echo ${c}'),
    ('cmd', 'set c 5 ; inc c 3 6 ; echo ${c}'), ('cmd', 'set c 5 ; dec c 9 2 ; echo ${c}'), ('cmd', 'inc nocount ; echo ${nocount}'),
    ('cmd', 'chr 65'), ('cmd', 'tolower ABC def'), ('cmd', 'toupper abc DEF'), ('cmd', 'escape a\\b%c'),
    ('cmd', 'rand 1'), ('cmd', 'rand 0'),
    ('save and load: a word is written in the 7 syntax and read back (the backslash must not double)', None, None),
    ('cmd', 'set r1 "a\\b" ; save r.txt r1 ; clear r1 ; load r.txt ; array r1 0'),
    ('cmd', 'set r1 "a\\b" ; save r.txt r1 ; clear r1 ; load r.txt ; load r.txt ; array r1 1'),
    ('cmd', 'set r2 "x,y" ; save r.txt r2 ; clear r2 ; load r.txt ; size r2'),
    ('cmd', 'set r3 "say \\"hi\\"" ; save r.txt r3 ; clear r3 ; load r.txt ; array r3 0'),
    ('cmd', 'set r4 "$(if 1 a b)" ; save r.txt r4 ; clear r4 ; load r.txt ; get r4 0'),
    ('cmd', 'set r4 "$(if 1 a b)" ; save r.txt r4 ; clear r4 ; load r.txt ; array r4 0'),
    ('cmd', 'set r5 "${b}${0}" ; save r.txt r5 ; clear r5 ; load r.txt ; array r5 0'),
    ('cmd', 'adddict r6 one ; adddict r6 "t w o" ; save r.txt r6 ; clear r6 ; load r.txt ; enumerate r6'),
    ('cmd', 'set r7 secret ; savecrypt r.txt r7 ; clear r7 ; load r.txt ; array r7 0'),
    ('cmd', 'set r8 "a b" ; get r8 0'),
    ('files and splitting', None, None),
    ('cmd', 'textload t.txt tx ; echo ${tx.1}/${tx.2}/${tx.3}/${tx.size}'),
    ('cmd', 'textload nosuch.txt ty ; size ty.size'),
    ('cmd', 'split sp "1ch2ch3" ch ; echo ${sp.1}/${sp.2}/${sp.3}/${sp.size}'),
    ('cmd', 'split sq "a,,b" "," ; echo ${sq.1}/${sq.2}/${sq.3}/${sq.size}'),
    ('cmd', 'split sr "abc" "x" ; echo ${sr.1}/${sr.size}'),
    ('cmd', 'split ss "" "x" ; size ss.size'),
    ('cmd', 'readdir . rd ; if $([ $(size rd) -gt 2 ]) many few'),
    ('cmd', 'readdir nosuchdir rd2 ; size rd2'),
    ('settings in kawari.ini (include is read at its place: the words of ord come in the order A B C)', None, None),
    ('echo', '${sv}'), ('echo', '${av}'), ('echo', '${tl.1}/${tl.size}'), ('cmd', 'enumerate ord'),
    ('piro', None, None),
    ('cmd', 'pirocall #nolabel dflt'), ('cmd', 'pirocall #nolabel'),
]

work = tempfile.mkdtemp(prefix='gen7_')
try:
    for f in os.listdir(FIXTURE):
        if f != 'cases.txt':
            shutil.copy(os.path.join(FIXTURE, f), work)
    lines = []
    count = 0
    for item in CASES:
        if item[1] is None and len(item) == 3:
            lines.append('')
            lines.append('# ' + item[0])
            continue
        verb, text = item
        session = (('.\n' if verb == 'cmd' else '') + text + '\nexit\n').encode('ascii')
        p = subprocess.run([KOSUI7], input=session, cwd=work, capture_output=True, timeout=20)
        out = p.stdout.decode('latin-1').replace('\r\n', '\n')
        i = out.find('mode change by "."\n')
        rest = out[i + len('mode change by "."\n'):]
        m = re.match(r'(?s)^(?:(?:echo|command)-mode > )+(.*?)\n(?:echo|command)-mode > $', rest)
        if not m:
            print('cannot parse: ' + repr((verb, text, rest)))
            sys.exit(1)
        result = m.group(1)
        if '\n' in result:
            print('multi-line output, skipped: ' + repr((verb, text, result)))
            continue
        lines.append('%s %s' % (verb, text))
        lines.append('= ' + result if result else '=')
        count += 1
finally:
    shutil.rmtree(work, ignore_errors=True)

header = '''# Cases for the kosui7 suite (the Kawari 7 compatible mode).
#
#   echo <input>   input line in echo mode (evaluated as a sentence)
#   cmd <input>    input line in command mode (evaluated as $(<input>))
#   = <text>       expected output (a lone "=" is an empty line)
#
# The folder has kawari.ini and no kawarirc.kis, so kosui runs in the Kawari 7 compatible mode.
# Every case runs in a fresh copy of this folder (without cases.txt).
#
# The expected outputs of the sections before "extensions" were taken from the real Kawari 7.5.0
# (kosui.exe 7.5.0): the same lines gave the same output there. Use ASCII only.
'''
extension = '''
# extensions (not in 7.5.0): the 9 syntax is allowed in a 7 ghost. A $[ ] expression and a =kis ... =end zone
# are read in the 9 grammar to the end. A word that is not in such a place stays in the 7 grammar.
echo $[ 1 + 2 ]
= 3
echo $[ ${n} * 2 ]
= 6
echo $[ "a]b" == "a]b" ]
= true
echo x$[ 2 * 3 ]y
= x6y
echo ${zone2}
= 6
echo ${from_zone}
= 9-syntax
cmd setstr s1 "a\\b" ; save r.txt s1 ; clear s1 ; load r.txt ; get s1 0
= "a\\b"
cmd adddict s2 "$[ 1 + 2 ]" ; save r.txt s2 ; clear s2 ; load r.txt ; array s2 0
= 3
'''
open(OUT, 'wb').write((header + '\n'.join(lines) + '\n' + extension).encode('ascii'))
print('wrote %d cases from kosui 7.5.0' % count)
