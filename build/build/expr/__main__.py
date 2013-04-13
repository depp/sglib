from . import evaluate, EvalError
import unittest

ENV = {
    'os': 'amiga',
    'flags': {'cheese': True, 'sub': {'flag': 'abc'}},
}

TEST_SUCCESS = [
    ('abc', 'abc'),
    ('   abc   ', 'abc'),
    ('"abc"', 'abc'),
    ('"abc\\ndef"', 'abc\ndef'),
    ('concat abc def', 'abcdef'),
    ('$os==amiga', True),
    ('$os == irix', False),
    ('$flags.cheese', True),
    ('$flags.sub.flag', 'abc'),
]

class Test(unittest.TestCase):
    def test_success(self):
        for x, y in TEST_SUCCESS:
            z = evaluate(ENV, x)
            self.assertEqual(y, z)

if __name__ == '__main__':
    unittest.main()
