#!/usr/bin/env python3
"""Build and verify every phase gate, with exact output and process status."""
from pathlib import Path
import subprocess, re, json, time, argparse, sys, hashlib
ROOT=Path(__file__).resolve().parent
EXPECT={
 'p0_write':(b'abc|0123456789|X\n',0),
 'p1_exit':(b'survived timer interrupts\n',7),
 'p2_fork':(b''.join(('ok %02d\n'%n).encode() for n in [4,5,1,2,3,6,7,8]),0),
 'p2_exit':(b'ok 01\nok 02\n',5),
 'p3_registry':(b''.join(('ok %02d\n'%n).encode() for n in range(1,9)),0),
 'p4_transfer':(b''.join(('ok %02d\n'%n).encode() for n in range(1,34)),0),
 'p4_temporary':(b'ok 01\nok 02\nok 03\n',0),
}
BY_PHASE=[['p0_write'],['p1_exit'],['p2_fork','p2_exit'],['p3_registry'],['p4_transfer','p4_temporary']]

def payloads(data):
    stack=[]; completed=[]
    for line in data.splitlines(keepends=True):
        if re.search(rb'>{32,}',line): stack.append([])
        elif re.search(rb'<{32,}',line):
            if stack:
                value=b''.join(stack.pop())
                if value.startswith(b'\n'): value=value[1:]
                if value.endswith(b'\n'): value=value[:-1]
                completed.append(value)
        elif stack: stack[-1].append(line)
    return completed

def run(binary,args,folder,name,code=0,expected=None,nested=False,timeout=90):
    t=time.monotonic()
    try:
        proc=subprocess.run([str(binary),*args],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout)
        data=proc.stdout; status=proc.returncode
    except subprocess.TimeoutExpired as e: data=e.stdout or b''; status='TIMEOUT'
    (folder/(name+'.log')).write_bytes(data)
    diagnostics=bool(re.search(rb'\b(?:error|warning)\b',data,re.I))
    good=status==code and not diagnostics and b'FAIL' not in data
    if expected is not None: good=good and expected in payloads(data)
    endings=re.findall(rb'terminating[^\n]*exit code (-?\d+)',data)
    if nested: good=good and endings==[str(code).encode(),b'0']
    return dict(test=name,passed=bool(good),exit_code=status,expected_exit=code,seconds=round(time.monotonic()-t,3),termination_codes=[int(x) for x in endings])

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--phase',default='all',choices=['all','0','1','2','3','4']); args=ap.parse_args()
    phases=range(5) if args.phase=='all' else [int(args.phase)]
    results=[]
    for phase in phases:
        folder=ROOT/'evidence'/('phase'+str(phase));folder.mkdir(parents=True,exist_ok=True)
        build=ROOT/'build';build.mkdir(exist_ok=True)
        source=ROOT/'checkpoints'/('phase'+str(phase)+'.c')
        if phase==4: source=ROOT/'selfie.c'
        binary=build/('selfie-phase'+str(phase))
        cp=subprocess.run(['gcc','-w','-O2','-m64','-Duint64_t=unsigned long',str(source),'-o',str(binary)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (folder/'build.log').write_bytes(cp.stdout)
        rows=[dict(test='gcc_build',passed=cp.returncode==0,exit_code=cp.returncode)]
        if cp.returncode==0:
            rows.append(run(binary,['-c',str(source)],folder,'self_compile'))
            rows.append(run(binary,['-c',str(source),'-m','64','-c','hello.c','-m','1'],folder,'self_on_self',expected=b'hello from selfie\n',nested=True))
            for name in sum(BY_PHASE[:phase+1],[]):
                expected,code=EXPECT[name]
                rows.append(run(binary,['-c','tests/'+name+'.c','-m','2'],folder,name,code,expected))
        results.append(dict(phase=phase,passed=all(r['passed'] for r in rows),checks=rows))
        print('PHASE',phase, 'PASS' if results[-1]['passed'] else 'FAIL',[(r['test'],r.get('exit_code')) for r in rows if not r['passed']],flush=True)
        (folder/'results.json').write_text(json.dumps(results[-1],indent=2)+'\n')
        if not results[-1]['passed']: break
    (ROOT/'evidence/gates.json').write_text(json.dumps(results,indent=2)+'\n')
    return 0 if all(r['passed'] for r in results) else 1
if __name__=='__main__':sys.exit(main())
