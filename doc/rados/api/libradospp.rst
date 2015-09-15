==================
 LibradosPP (C++)
==================

.. highlight:: c++

`librados` provides low-level access to the RADOS service. For an
overview of RADOS, see :doc:`../../architecture`.

Example: connecting and writing an object
=========================================

In order to use the Ceph C++ API you need to include the rados/librados.hpp
header and link to librados.

You then need to create an object of type librados::Rados and initialise it::

  librados::Rados rados;
  ret = rados.init("admin"); // just use the client.admin keyring
  if (ret < 0) {
    std::cerr << "couldn't initialize rados! error " << ret << std::endl;

