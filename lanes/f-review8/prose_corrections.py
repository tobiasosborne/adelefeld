from pathlib import Path
p=Path('lanes/f-review8/report_body.txt')
s=p.read_text().replace('clamped sums have magnitude at most 2^63-1 before\nclamping,',
'''when INF is not absorbed the sum lies between -2^63 and 2^63-2 before clamping,''')
p.write_text(s)
p=Path('lanes/f-review8/progress.md')
s=p.read_text()
s=s.replace('clamped sums fit because 2*2^62<2^63;',
'''clamped sums fit the signed range (the lower endpoint -2^63 is admitted,
and the positive INF shortcut excludes +2^63);''')
p.write_text(s)
