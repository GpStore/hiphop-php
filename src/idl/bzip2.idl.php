<?php

include_once 'base.php';

///////////////////////////////////////////////////////////////////////////////
// bzip2 functions

f('bzclose', Variant,
  array('bz' => Resource));

f('bzcompress', Variant,
  array('source' => String,
        'blocksize' => array (Int32, '4'),
        'workfactor' => array (Int32, '0')));

f('bzdecompress', Variant,
  array('source' => String,
        'small' => array (Int32, '0')));

f('bzerrno', Int32,
  array('bz' => Resource));

f('bzerror', Variant,
  array('bz' => Resource));

f('bzerrstr', String,
  array('bz' => Resource));

f('bzflush', Variant,
  array('bz' => Resource));

f('bzopen', Variant,
  array('filename' => String,
        'mode' => String));

f('bzread', Variant,
  array('bz' => Resource,
        'length' => array(Int32, '1024')));

f('bzwrite', Variant,
  array('bz' => Resource,
        'data' => String,
        'length' => array(Int32, '0')));

