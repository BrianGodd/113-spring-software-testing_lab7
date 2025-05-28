Name: 黃永恩
Student ID:111550083

### Crash Fuzzer Report
```
            american fuzzy lop ++4.01c {default} (./program) [fast]
┌─ process timing ────────────────────────────────────┬─ overall results ────┐
│        run time : 0 days, 0 hrs, 1 min, 20 sec      │  cycles done : 15    │
│   last new find : 0 days, 0 hrs, 1 min, 11 sec      │ corpus count : 7     │
│last saved crash : 0 days, 0 hrs, 0 min, 23 sec      │saved crashes : 1     │
│ last saved hang : none seen yet                     │  saved hangs : 0     │
├─ cycle progress ─────────────────────┬─ map coverage┴──────────────────────┤
│  now processing : 1.20 (14.3%)       │    map density : 30.77% / 76.92%    │
│  runs timed out : 0 (0.00%)          │ count coverage : 39.40 bits/tuple   │
├─ stage progress ─────────────────────┼─ findings in depth ─────────────────┤
│  now trying : splice 14              │ favored items : 7 (100.00%)         │
│ stage execs : 20/36 (55.56%)         │  new edges on : 7 (100.00%)         │
│ total execs : 142k                   │ total crashes : 1 (1 saved)         │
│  exec speed : 1797/sec               │  total tmouts : 0 (0 saved)         │
├─ fuzzing strategy yields ────────────┴─────────────┬─ item geometry ───────┤
│   bit flips : disabled (default, enable with -D)   │    levels : 6         │
│  byte flips : disabled (default, enable with -D)   │   pending : 0         │
│ arithmetics : disabled (default, enable with -D)   │  pend fav : 0         │
│  known ints : disabled (default, enable with -D)   │ own finds : 6         │
│  dictionary : n/a                                  │  imported : 0         │
│havoc/splice : 5/58.8k, 2/83.2k                     │ stability : 100.00%   │
│py/custom/rq : unused, unused, unused, unused       ├───────────────────────┘
│    trim/eff : 0.00%/7, disabled                    │          [cpu000: 25%]
└────────────────────────────────────────────────────┘
```

### Crash Input (use `xxd`)
```
xxd output/default/crashes/id:000000,sig:06,src:000004,time:56655,execs:100249,op:havoc,rep:4
00000000: 4655 5a5a 210a                           FUZZ!.
```
