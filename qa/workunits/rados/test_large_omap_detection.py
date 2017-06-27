#!/usr/bin/python2.7
# -*- mode:python -*-
# vim: ts=4 sw=4 smarttab expandtab
#
# Copyright (C) 2015, 2016, 2017 Red Hat <contact@redhat.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library Public License for more details.
#

import subprocess
import shlex
import time
import rados

def cleanup(cluster):
    cluster.delete_pool('large-omap-test-pool')
    cluster.shutdown()

def init():
    cluster = rados.Rados(conffile='/etc/ceph/ceph.conf')
    cluster.connect()
    print "\nCluster ID: " + cluster.get_fsid()
    cluster.create_pool('large-omap-test-pool')
    ioctx = cluster.open_ioctx('large-omap-test-pool')
    ioctx.write_full('large-omap-test-object1', "Lorem ipsum")
    op = ioctx.create_write_op()

    keys = []
    values = []
    for x in xrange(20001):
        keys.append(str(x))
        values.append("X")

    ioctx.set_omap(op, tuple(keys), tuple(values))
    ioctx.operate_write_op(op, 'large-omap-test-object1', 0)
    ioctx.release_write_op(op)

    ioctx.write_full('large-omap-test-object2', "Lorem ipsum dolor")
    op = ioctx.create_write_op()

    buffer = ("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
              "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
              "enim ad minim veniam, quis nostrud exercitation ullamco laboris "
              "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
              "reprehenderit in voluptate velit esse cillum dolore eu fugiat "
              "nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
              "sunt in culpa qui officia deserunt mollit anim id est laborum.")

    keys = []
    values = []
    for x in xrange(20000):
        keys.append(str(x))
        values.append(buffer)

    ioctx.set_omap(op, tuple(keys), tuple(values))
    ioctx.operate_write_op(op, 'large-omap-test-object2', 0)
    ioctx.release_write_op(op)
    ioctx.close()
    return cluster

def scrub():
    command = "ceph osd deep-scrub osd.0"
    command1 = "ceph osd deep-scrub osd.1"
    subprocess.check_call(shlex.split(command))
    subprocess.check_call(shlex.split(command1))

def check_health_output():
    output = subprocess.check_output(["ceph", "health", "detail"])
    result = 0
    for line in output.splitlines():
        result += int(line.find('2 large omap objects') != -1)

    if result != 2:
        print "Error, got invalid output:"
        print output
        raise Exception

def main():
    cluster = init()
    scrub()
    # I would hope this would be long enough for scrub to complete
    time.sleep(60)
    check_health_output()

    cleanup(cluster)

if __name__ == '__main__':
    main()
