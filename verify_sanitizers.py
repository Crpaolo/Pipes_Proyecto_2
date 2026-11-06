#!/usr/bin/env python3
"""Exercise new memory paths with AddressSanitizer and UndefinedBehaviorSanitizer."""
from pathlib import Path
import subprocess,os,json,sys
from verify import ROOT,EXPECT,payloads
from verify_extra import EXTRA

def main():
    folder=ROOT/'evidence/sanitizers';folder.mkdir(parents=True,exist_ok=True)
    binary=ROOT/'build/selfie-sanitized';binary.parent.mkdir(exist_ok=True)
    cp=subprocess.run(['gcc','-w','-O1','-g','-m64','-fno-omit-frame-pointer','-fsanitize=address,undefined','-Duint64_t=unsigned long','selfie.c','-o',str(binary)],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (folder/'build.log').write_bytes(cp.stdout)
    if cp.returncode: return cp.returncode
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    rows=[]
    for name in ['p2_fork','p4_transfer','x_console_binary','x_pipe_binary','x_pipe_edges','x_pipe_boundaries','x_pipe_wrap','x_fork_ipc']:
        expected,code=EXPECT[name] if name in EXPECT else (EXTRA[name],0)
        cp=subprocess.run([str(binary),'-c','tests/'+name+'.c','-m','8'],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90,env=env)
        (folder/(name+'.log')).write_bytes(cp.stdout)
        good=cp.returncode==code and expected in payloads(cp.stdout) and b'FAIL' not in cp.stdout and b'runtime error:' not in cp.stdout and b'ERROR: AddressSanitizer' not in cp.stdout
        rows.append(dict(test=name,passed=good,exit_code=cp.returncode))
        print('SANITIZER',name,'PASS' if good else 'FAIL',flush=True)
    result=dict(passed=all(x['passed'] for x in rows),checks=rows,note='Known arena leaks excluded; guest buffer bounds checked by functional tests.')
    (ROOT/'evidence/sanitizers.json').write_text(json.dumps(result,indent=2)+'\n')
    return 0 if result['passed'] else 1
if __name__=='__main__':sys.exit(main())
