#!/usr/bin/env python3
"""
Checker helper for PCom / HTTP client assignment (#4)

Dependencies: pexpect
"""

import sys
from collections import namedtuple
import argparse
import re

import pexpect

TEST_USERNAME = "test"
TEST_PASSWORD = "test123"
EXPECT_TIMEOUT = 1  # 1 second should be enough...

RE_SUCCESS = r"[Ss]uccess?"
# JSON parsing using RegExp: don't do this at home!
RE_BOOK_ID = r"id\s*=\s*([0-9]+)|\"id\"\s*:\s*\"?([0-9]+)"
RE_EXTRACT_FIELD = r"%s\s*=\s*(.+)\s*|\"%s\"\s*:\s*(?:\"([^\"]+)|([0-9]+))"


# classes used
class CheckerException(Exception): pass

class ExpectInputWrapper:
    """ Utility IO class used for prefixing the debug output. """
    def __init__(self, prefix):
        self.prefix = prefix
    def write(self, s):
        print("%s%s" % (self.prefix, s.strip()))
    def flush(self): pass


def normalize_user(xargs):
    user = xargs.get("user")
    if user and isinstance(user, str):
        user_pair = user.split(":")
        xargs["user"] = { "username": user_pair[0], "password": user_pair[1] }
    if not xargs.get("user"):
        xargs["user"] = { "username": TEST_USERNAME, "password": TEST_PASSWORD }

def expect_send_params(p, xvars):
    keys = list(xvars.keys())
    xpatterns =  [(r"(" + kw + r")\s*=\s*") for kw in keys]
    xseen = set()
    while xseen != set(keys):
        idx = p.expect(xpatterns)
        p.sendline(str(xvars.get(keys[idx])))
        xseen.add(keys[idx])

def extract_book_ids(output):
    matches = re.findall(RE_BOOK_ID, output)
    return [val[0] or val[1] for val in matches]

def extract_book_fields(output, field):
    matches = re.findall(RE_EXTRACT_FIELD % (field, field), output)
    return [val[0] or val[1] or val[2] for val in matches]

def do_register(p, xargs):
    normalize_user(xargs)
    p.sendline("register")
    expect_send_params(p, xargs["user"])
    p.expect([RE_SUCCESS, pexpect.TIMEOUT])
    print(p.after)

def do_login(p, xargs):
    normalize_user(xargs)
    p.sendline("login")
    expect_send_params(p, xargs["user"])
    p.expect([RE_SUCCESS, pexpect.TIMEOUT])
    print(p.after)

def do_enter(p, xargs):
    p.sendline("enter_library")
    p.expect([RE_SUCCESS, pexpect.TIMEOUT])
    print(p.after)

def do_get_books(p, xargs):
    p.sendline("get_books")
    p.expect(pexpect.TIMEOUT)
    xargs["book_ids"] = extract_book_ids(p.before)
    xargs["book_titles"] = extract_book_fields(p.before, "title")
    print("Retrieved book IDs + titles:", xargs["book_ids"], xargs["book_titles"])
    expect_count = xargs.get("expect_count", False)
    if type(expect_count) is int:
        if len(xargs["book_ids"]) != expect_count:
            raise CheckerException("Book count mismatch: %s != %s" % 
                                   (len(xargs["book_ids"]), expect_count))

def do_add_book(p, xargs):
    book_titles = xargs.get("book_titles", [])
    book = xargs.get("book", {})
    if book["title"] in book_titles:
        print("SKIP: book already added!")
        return
    p.sendline("add_book")
    book_struct = { key : book.get(key, "") for key in ("title", "author", "genre", "publisher", "page_count") }
    expect_send_params(p, book_struct)
    p.expect([RE_SUCCESS, pexpect.TIMEOUT])

def do_get_book_id(p, xargs):
    p.sendline("get_book")
    book_ids = xargs.get("book_ids")
    idx = xargs.get("book_idx")
    if not book_ids:
        raise CheckerException("No books found!")
    if idx >= len(book_ids):
        raise CheckerException("No book index: %s" % str(idx))
    book_id = book_ids[idx]
    expect_send_params(p, {"id": book_id})
    p.expect([RE_SUCCESS, pexpect.TIMEOUT])
    expect_book = xargs.get("expect_book", False)
    if type(expect_book) is dict:
        for field in expect_book.keys():
            values = extract_book_fields(p.before, field)
            if not values:
                raise CheckerException("Cannot find book field '%s'" % field)
            if len(values) != 1:
                raise CheckerException("Multiple '%s' fields found in output!" % field)
            if str(values[0]) != str(expect_book[field]):
                raise CheckerException("Book field '%s' mismatch: %s != %s" % 
                                    (field, values[0], expect_book[field]))

def do_delete_book(p, xargs):
    p.sendline("delete_book")
    book_ids = xargs.get("book_ids")
    idx = xargs.get("book_idx")
    if not book_ids:
        raise CheckerException("No books found!")
    if idx >= len(book_ids):
        raise CheckerException("No book index: %s" % str(idx))
    book_id = book_ids[idx]
    expect_send_params(p, {"id": book_id})
    p.expect([RE_SUCCESS, pexpect.TIMEOUT])

def do_logout(p, xargs):
    p.sendline("logout")
    p.expect([RE_SUCCESS, pexpect.TIMEOUT])

def do_exit(p, xargs):
    p.sendline("exit")
    p.expect(pexpect.EOF)


ACTIONS = {
    "register": do_register,
    "login": do_login,
    "enter_library": do_enter,
    "get_books": do_get_books,
    "get_book_id": do_get_book_id,
    "add_book": do_add_book,
    "delete_book": do_delete_book,
    "logout": do_logout,
    "exit": do_exit,
}

SCRIPTS = {
    "full": [
        ("register", {}),  # use CLI-provided user
        ("login", {}), ("enter_library", {}),
        ("get_books", {"expect_count": False}),
        ("add_book", {"book": {
                "title": "Computer Networks", "author": "A. Tanenbaum et. al.",
                "genre": "Manual", "publisher": "Prentice Hall", "page_count": 950,
            }}),
        ("add_book", {"book": {
                "title": "Viata Lui Nutu Camataru: Dresor de Lei si de Fraieri",
                "author": "Codin Maticiuc", "genre": "Lifestyle", "publisher": "Scoala Vietii",
                "page_count": 200,
            }}),
        ("get_books", {"expect_count": 2}),
        ("get_book_id", {"book_idx": 0, "expect_book": {"title": "Computer Networks", "page_count": 950}}),
        ("delete_book", {"book_idx": 1}),
        ("logout", {}), ("exit", {}), 
    ],
    "readonly": [
        ("register", {}),  # use CLI-provided user
        ("login", {}), ("enter_library", {}),
        ("get_books", {"expect_count": 2}),
        ("get_book_id", {"book_idx": 0, "expect_book": {"title": "Computer Networks", "page_count": 950}}),
        ("logout", {}), ("exit", {}), 
    ]
}


def run_tasks(p, args):
    script_name = args.script or "full"
    if script_name not in SCRIPTS:
        raise CheckerException("Invalid script: %s" % (script_name))
    script = SCRIPTS[script_name]
    xargs = dict(vars(args))
    for action, params in script:
        params = dict(params)  # copy the original dict
        ignore = params.pop("ignore", False)
        if params:
            xargs.update(params)
        try:
            print("%s: " % (action,))
            ACTIONS[action](p, xargs)
        except CheckerException as ex:
            ex = CheckerException("%s: %s" % (action, str(ex)))
            sys.stderr.write(str(ex) + "\n")
            if ignore:
                continue
            raise ex
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='checker.py',
                                     description='Python helper for PCom / HTTP Assignment')
    parser.add_argument('program')
    parser.add_argument('--script', choices=SCRIPTS.keys())
    parser.add_argument('-u', '--user', help="Override username & password " \
            "(separated by colon, e.g. `-u user:pass`")
    parser.add_argument('-d', '--debug', help="Enable debug output", action="store_true")

    args = parser.parse_args()
    p = pexpect.spawn(args.program, encoding='utf-8', echo=False, timeout=EXPECT_TIMEOUT)
    if args.debug:
        p.logfile_send = ExpectInputWrapper("  > ")
        p.logfile_read = ExpectInputWrapper("  < ")
    run_tasks(p, args)

