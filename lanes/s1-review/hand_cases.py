#!/usr/bin/env python3
"""Send hand-constructed certificates, field by field, to the public verifier."""
import subprocess


def line(r, c, n, a, b, kind, g, e, v, x0, y):
    fields = [r, c, n, kind] + [x for row in a for x in row] + b
    for rows, width in ((g, c), (e, r), (v, c), (x0, 1 if kind == 0 else 0),
                        (y, 0 if kind == 0 else 1)):
        fields.extend((len(rows), width))
        fields.extend(x for row in rows for x in row)
    return ' '.join(map(str, fields)) + '\n'


def main():
    cases = [
        ('true free kernel', 1, 1, 4, [[0]], [0], 0, [[1]], [], [], [[0]], [], (1, 1)),
        ('missing generator', 1, 1, 4, [[0]], [0], 0, [], [[1]], [[0]], [[0]], [], (1, 0)),
        ('wrong image and kernel', 1, 1, 4, [[0]], [0], 0, [[2]], [[2]], [[0]], [[0]], [], (1, 0)),
        ('false non-solution', 1, 1, 4, [[0]], [1], 1, [[1]], [], [], [], [[0]], (1, 0)),
        ('wrong kernel', 1, 1, 4, [[2]], [0], 0, [[1]], [], [], [[0]], [], (1, 0)),
        ('wrong particular', 1, 1, 4, [[1]], [0], 0, [], [[1]], [[1]], [[1]], [], (1, 0)),
        ('true non-solution', 1, 1, 4, [[0]], [1], 1, [[1]], [], [], [], [[1]], (1, 1)),
        ('zero row in G', 1, 1, 4, [[0]], [0], 0, [[0]], [], [], [[0]], [], (0, 0)),
    ]
    inp = ''.join(line(*case[1:-1]) for case in cases)
    run = subprocess.run(['lanes/s1-review/verify_probe'], input=inp, text=True,
                         capture_output=True, check=True)
    outputs = [tuple(map(int, row.split())) for row in run.stdout.splitlines()]
    for case, output in zip(cases, outputs):
        assert output == case[-1], (case[0], output, case[-1])
        print(case[0] + ': canonical=' + str(output[0]) + ' verify=' + str(output[1]))
    assert len(outputs) == len(cases)
    print('cases=8 unexpected=0')


if __name__ == '__main__':
    main()
