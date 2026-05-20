# -*- encoding: utf-8 -*-
import sys

if sys.version.startswith('2'):
    from py2_helper import * # noqa
else:
    from .py3_helper import * # noqa