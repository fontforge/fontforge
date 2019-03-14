#!/usr/bin/python

from __future__ import print_function
from shutil import copy2
import argparse
import os
import re

URL_RE = re.compile(r'(href|src)="([^"]+)"', re.IGNORECASE)
BODY_RE = re.compile(r'<BODY([^/>]*)>(\r?\n)', re.IGNORECASE)
CSS_URL_RE = re.compile(r'url\("([^"]+)"\)', re.IGNORECASE)

BANNER = '''<div style="margin:0; height: 4	em; padding: 0.5em; background: red; color:yellow; text-align:center; font-size:1em; font-family: sans-serif;">
	<p><a href="http://fontforge.github.io" style="padding: 0.5em; color: yellow; font-weight: bold; text-decoration: none;" onmouseover="this.style.background='black';" onmouseout="this.style.background='red';" >This is part of the old website. New website begins at fontforge.github.io</a></p>
	<p><a href="https://github.com/fontforge/fontforge.github.io" style="padding: 0.5em; color: yellow; font-weight: bold; text-decoration: none;" onmouseover="this.style.background='black';" onmouseout="this.style.background='red';" >Are you a web developer? Help us migrate this page on Github</a></p>
</div>'''


def mkdir(path):
    try:
        os.makedirs(path)
    except OSError:
        pass


def rewrite_html(path, destination, root):
    def href_updater(match):
        url = match.group(2)
        if '#' in url or '://' in url or '.html' in url or ('.' not in url) or url.startswith('mailto:'):
            return match.group(0)
        name, ext = os.path.splitext(url)
        if ext.lower() in ('.gif', '.jpeg', '.png'):
            fixed_url = os.path.normpath(
                os.path.join('/assets/img/old' + root, url))
            if not os.path.exists(destination + fixed_url):
                #print(url, root, fixed_url)
                pass
        elif ext.lower() in ('.css',):
            # REWRITE URLS
            fixed_url = os.path.normpath(
                os.path.join('/assets/css/old' + root, url))
        else:
            fixed_url = os.path.normpath(
                os.path.join('/assets/old' + root, url))
        return '{}="{}"'.format(match.group(1), fixed_url)

    def body_updater(match):
        if 'sidebar' in match.group(1):
            return match.group(0)
        return match.group(0) + BANNER + match.group(2)

    mkdir(destination + root)
    dest_path = os.path.join(destination + root, os.path.basename(path))
    with open(path, 'rb') as fpi:
        data = fpi.read()
        data = URL_RE.sub(href_updater, data)
        data = BODY_RE.sub(body_updater, data, 1)

        with open(dest_path, 'wb') as fpo:
            fpo.write(data)


def rewrite_css(path, destination, root):
    def url_updater(match):
        fixed_url = os.path.normpath(os.path.join(
            '/assets/img/old' + root, match.group(1)))
        return 'url("{}")'.format(fixed_url)

    mkdir(destination + root)
    dest_path = os.path.join(destination + root, os.path.basename(path))
    with open(path, 'rb') as fpi:
        data = fpi.read()
        data = CSS_URL_RE.sub(url_updater, data)

        with open(dest_path, 'wb') as fpo:
            fpo.write(data)


def copy_to(src, dst):
    mkdir(dst)
    copy2(src, dst)


def main(source, destination):
    for root, _, files in os.walk(source):
        for file in files:
            if file[0] == '.' or file.startswith('Makefile'):
                continue
            file = os.path.join(root, file)
            stripped_root = root[len(source):]
            name, ext = os.path.splitext(file)
            if ext.lower() == '.html':
                if root == source and os.path.basename(file) == 'index.html':
                    continue #skip
                rewrite_html(file, destination, stripped_root)
            elif ext.lower() in ('.gif', '.jpeg', '.png'):
                copy_to(file, destination + '/assets/img/old' + stripped_root)
            elif ext.lower() in ('.css',):
                rewrite_css(file, destination +
                            '/assets/css/old', stripped_root)
            else:
                copy_to(file, destination + '/assets/old' + stripped_root)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Synchronise docs with the published website layout.',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--source', dest='source', type=str, default='html',
                        help='the source directory')
    parser.add_argument('destination', type=str,
                        help='the root of the website repository')

    args = parser.parse_args()
    main(args.source, args.destination)
