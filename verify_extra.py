#!/usr/bin/env python3
"""Boundary, binary, IPC, repeated-run and nested checks for final phase 4."""
from pathlib import Path
import json,sys,subprocess,re
from verify import ROOT,run,EXPECT

def checks(n):return b''.join(('ok %02d\n'%i).encode() for i in range(1,n+1))
EXTRA={
'x_console_binary':b'A\0B\xff',
'x_pipe_binary':checks(12),
'x_pipe_edges':checks(28),
'x_pipe_boundaries':checks(22),
'x_pipe_wrap':checks(3),
'x_fork_ipc':checks(13),
'x_reset_registry':b'',
}

def main():
    binary=ROOT/'build/selfie-phase4';folder=ROOT/'evidence/extra';folder.mkdir(parents=True,exist_ok=True);rows=[]
    for name,expected in EXTRA.items():
        row=run(binary,['-c','tests/'+name+'.c','-m','8'],folder,name,expected=expected)
        rows.append(row);print(name,'PASS' if row['passed'] else 'FAIL',row['exit_code'],flush=True)
    # The inherited CLI returns after its first -m; use a native harness to
    # exercise two reset_microkernel cycles in one host process instead.
    harness=ROOT/'build/reset-kernel'
    cp=subprocess.run(['gcc','-w','-O2','-m64','-Duint64_t=unsigned long','tests/x_reset_kernel.c','-o',str(harness)],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (folder/'reset_kernel_build.log').write_bytes(cp.stdout)
    proc=subprocess.run([str(harness)],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT) if cp.returncode==0 else cp
    (folder/'reset_kernel.log').write_bytes(proc.stdout)
    row=dict(test='reset_kernel',passed=cp.returncode==0 and proc.returncode==0 and proc.stdout==b'ok reset\n',exit_code=proc.returncode)
    rows.append(row);print('reset_kernel','PASS' if row['passed'] else 'FAIL',flush=True)
    # Confirm real pipe transfers and fork/scheduler below the self-hosted VM.
    for name in ['p3_registry','p4_transfer','p4_temporary','x_pipe_binary','x_pipe_boundaries','x_fork_ipc','x_console_binary']:
        if name in EXPECT: expected,code=EXPECT[name]
        else:expected,code=EXTRA[name],0
        # Outer exit returns inner nonzero status, e.g. p2_exit's expected 5.
        row=run(binary,['-c','selfie.c','-m','128','-c','tests/'+name+'.c','-m','8'],folder,'nested_'+name,code=code,expected=expected,timeout=180)
        row['passed']=row['passed'] and row['termination_codes']==[code,code]
        rows.append(row);print('nested_'+name,'PASS' if row['passed'] else 'FAIL',row['exit_code'],flush=True)
    # Final source and checkpoint must be byte-for-byte identical.
    rows.append(dict(test='final_checkpoint_identity',passed=(ROOT/'selfie.c').read_bytes()==(ROOT/'checkpoints/phase4.c').read_bytes()))
    result=dict(passed=all(x['passed'] for x in rows),checks=rows)
    (ROOT/'evidence/extra_results.json').write_text(json.dumps(result,indent=2)+'\n')
    return 0 if result['passed'] else 1
if __name__=='__main__':sys.exit(main())
